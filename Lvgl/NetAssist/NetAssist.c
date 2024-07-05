#include "../lvgl.h"

#include <Library/Udp4SocketLib.h>
#include <Library/NetLib.h>

typedef struct {
	lv_obj_t *this;
  lv_obj_t *lb_title;
  lv_obj_t *lb_type;
  lv_obj_t *dd_type;
  lv_obj_t *lb_ip;
  lv_obj_t *ta_ip;
  lv_obj_t *lb_port;
  lv_obj_t *ta_port;
  lv_obj_t *sw;
} setting_network;

typedef struct {
	lv_obj_t *this;
  setting_network net;
} area_setting;

typedef struct {
	lv_obj_t *this;
  lv_obj_t *lb_send;
  lv_obj_t *lb_re_ip;
  lv_obj_t *ta_re_ip;
  lv_obj_t *lb_re_port;
  lv_obj_t *ta_re_port;
  lv_obj_t *btn_clr_send;
  lv_obj_t *lb_clr_send;
  lv_obj_t *btn_clr_log;
  lv_obj_t *lb_clr_log;
} data_config;

typedef struct {
	lv_obj_t *this;
  lv_obj_t *ta_send;
  lv_obj_t *btn_send;
  lv_obj_t *lb_send;
} data_control;

typedef struct {
	lv_obj_t *this;
  lv_obj_t *lb_data_log;
  lv_obj_t *ta_data_log;
  data_config conf;
  data_control ctrl;
} area_data;

typedef struct {
	lv_obj_t *win_main;
  lv_obj_t *header;
  lv_obj_t *usage;
  lv_obj_t *time;
  lv_timer_t *usage_timer;
	lv_obj_t *btn_cls;
  lv_style_t win_default;
  lv_style_t btn_cls_focused;
  lv_style_t btn_cls_clicked;
	lv_obj_t *content;
  area_setting setting;
  area_data data;
  lv_obj_t *lb_author;
} lv_ui;

lv_ui ui;
static UDP4_SOCKET *gSocketReceive = NULL;
static UDP4_SOCKET *gSocketTransmit = NULL;
static char fragment_buf[2048];

static void event_close(lv_event_t * e)
{
  lv_point_t point;
  lv_indev_get_point(lv_event_get_indev(e), &point);
  if (lv_obj_hit_test(lv_event_get_target_obj(e), &point)) {
    CloseUdp4Socket(gSocketReceive);
    CloseUdp4Socket(gSocketTransmit);
    lv_timer_delete(ui.usage_timer);
    lv_obj_delete(ui.win_main);
    lv_efi_app_exit();
  }
}

static void show_usage(lv_timer_t * t)
{
  EFI_TIME time;
  gRT->GetTime(&time, NULL);
  lv_label_set_text_fmt(ui.usage, "Usage: %3d%%", (100 - lv_timer_get_idle()));
  lv_label_set_text_fmt(ui.time, "Time: %4d.%2d.%2d:%2d.%2d.%2d", time.Year, time.Month, time.Day, time.Hour, time.Minute, time.Second);
}

static void send_event(lv_event_t * e)
{
  lv_point_t point;
  lv_indev_get_point(lv_event_get_indev(e), &point);
  if (lv_obj_hit_test(lv_event_get_target_obj(e), &point)) {
    const char *send_data = lv_textarea_get_text(ui.data.ctrl.ta_send);
    UINTN send_len = lv_strlen(send_data);
    if (send_len == 0)
      return;
    EFI_IPv4_ADDRESS ip4_remote;
    UINTN port_remote;
    const char *ip4_remote_str = lv_textarea_get_text(ui.data.conf.ta_re_ip);
    const char *port_remote_str = lv_textarea_get_text(ui.data.conf.ta_re_port);
    if (!NetLibAsciiStrToIp4(ip4_remote_str, &ip4_remote) == 0) {
      return;
    }
    port_remote = AsciiStrDecimalToUintn(port_remote_str);
    if (port_remote > 0xFFFF) {
      return;
    }
    gSocketTransmit->ConfigData.RemoteAddress = ip4_remote;
    gSocketTransmit->ConfigData.RemotePort = (UINT16)port_remote;
    gSocketTransmit->Udp4->Configure(gSocketTransmit->Udp4, NULL);
    gSocketTransmit->Udp4->Configure(gSocketTransmit->Udp4, &gSocketTransmit->ConfigData);
    EFI_UDP4_TRANSMIT_DATA *TxData = gSocketTransmit->TokenTransmit.Packet.TxData;
    ZeroMem(TxData, sizeof(EFI_UDP4_TRANSMIT_DATA));
    TxData->DataLength = send_len;
    TxData->FragmentCount = 1;
    TxData->FragmentTable[0].FragmentLength = send_len;
    TxData->FragmentTable[0].FragmentBuffer = (void *)send_data;
    gSocketTransmit->Udp4->Transmit(gSocketTransmit->Udp4, &gSocketTransmit->TokenTransmit);
    lv_textarea_set_text(ui.data.ctrl.ta_send, "");
  }
}

static void switch_event(lv_event_t * e)
{
  EFI_STATUS Status = 0;
  
  if (lv_obj_has_state(ui.setting.net.sw, LV_STATE_CHECKED)) {
    if (gSocketReceive == NULL) {
      return;
    }
    EFI_IPv4_ADDRESS ip4_station;
    UINTN port_station;
    const char *ip4_station_str = lv_textarea_get_text(ui.setting.net.ta_ip);
    const char *port_station_str = lv_textarea_get_text(ui.setting.net.ta_port);
    if (!NetLibAsciiStrToIp4(ip4_station_str, &ip4_station) == 0) {
      return;
    }
    port_station = AsciiStrDecimalToUintn(port_station_str);
    if (port_station > 0xFFFF) {
      return;
    }
    switch (lv_dropdown_get_selected(ui.setting.net.dd_type)) {
      case 0:   //Udp
        gSocketTransmit->ConfigData.StationAddress = ip4_station;
        gSocketTransmit->ConfigData.StationPort = (UINT16)port_station;
        gSocketReceive->ConfigData.StationPort = (UINT16)port_station;

        Status = gSocketReceive->Udp4->Configure(gSocketReceive->Udp4, NULL);
        Status = gSocketReceive->Udp4->Configure(gSocketReceive->Udp4, &gSocketReceive->ConfigData);
        Status = gSocketReceive->Udp4->Receive(gSocketReceive->Udp4, &gSocketReceive->TokenReceive);
        break;
    }
  } else {
    if (gSocketReceive == NULL) {
      return;
    }
    gSocketReceive->Udp4->Cancel(gSocketReceive->Udp4, NULL);
    gSocketTransmit->Udp4->Cancel(gSocketTransmit->Udp4, NULL);
  }
}

VOID EFIAPI Udp4ReceiveHandler(IN EFI_EVENT  Event,  IN VOID *Context)
{
  UDP4_SOCKET                 *Socket = Context;
  EFI_UDP4_RECEIVE_DATA       *RxData = Socket->TokenReceive.Packet.RxData;
  if (Socket->TokenReceive.Status == EFI_ABORTED || RxData->DataLength == 0 || gSocketTransmit == NULL)
    return;
  UINT32 BufferIndex = 0;
  
  lv_snprintf(fragment_buf, 2048, "%d.%d.%d.%d[%d]->%d.%d.%d.%d[%d]:\n", RxData->UdpSession.SourceAddress.Addr[0],
    RxData->UdpSession.SourceAddress.Addr[1],RxData->UdpSession.SourceAddress.Addr[2],RxData->UdpSession.SourceAddress.Addr[3],
    RxData->UdpSession.SourcePort, RxData->UdpSession.DestinationAddress.Addr[0],RxData->UdpSession.DestinationAddress.Addr[1],
    RxData->UdpSession.DestinationAddress.Addr[2],RxData->UdpSession.DestinationAddress.Addr[3],RxData->UdpSession.DestinationPort);
  lv_textarea_add_text(ui.data.ta_data_log, fragment_buf);
  for (UINT32 Index = 0; Index < RxData->FragmentCount; Index++) {
    CopyMem(fragment_buf + BufferIndex, RxData->FragmentTable[Index].FragmentBuffer, RxData->FragmentTable[Index].FragmentLength);
    BufferIndex += RxData->FragmentTable[Index].FragmentLength;
  }
  fragment_buf[RxData->DataLength] = '\n';
  fragment_buf[RxData->DataLength+1] = '\0';
  lv_textarea_add_text(ui.data.ta_data_log, fragment_buf);
  
  gBS->SignalEvent(RxData->RecycleSignal);
  Socket->Udp4->Receive(Socket->Udp4, &Socket->TokenReceive);
  return;
}

VOID EFIAPI Udp4NullHandler(IN EFI_EVENT  Event,  IN VOID *Context)
{

}

EFI_STATUS UdpInit(void)
{
  EFI_STATUS Status = 0;
  EFI_UDP4_CONFIG_DATA mConfigData = {
                          TRUE,  // AcceptBroadcast
                          FALSE,   // AcceptPromiscuous
                          FALSE,   // AcceptAnyPort
                          TRUE,   // AllowDuplicatePort
                          0,      // TypeOfService
                          16,      // TimeToLive
                          TRUE,   // DoNotFragment
                          0,      // ReceiveTimeout
                          0,      // TransmitTimeout
                          FALSE,  // UseDefaultAddress
                          {{0, 0, 0, 0}},  // StationAddress
                          {{0, 0, 0, 0}},  // SubnetMask
                          5566,      // StationPort
                          {{0, 0, 0, 0}},  // RemoteAddress
                          0,      // RemotePort
    };

    Status = CreateUdp4Socket(&mConfigData, (EFI_EVENT_NOTIFY)Udp4ReceiveHandler, (EFI_EVENT_NOTIFY)Udp4NullHandler, &gSocketReceive);
    if (!EFI_ERROR (Status)) {
        gSocketReceive->Udp4->Receive(gSocketReceive->Udp4, &gSocketReceive->TokenReceive);
    }
    *(UINT32 *)mConfigData.StationAddress.Addr = (192 | 168 << 8 | 101 << 16 | 17 << 24);
    *(UINT32 *)mConfigData.SubnetMask.Addr = (255 | 255 << 8 | 255 << 16 | 0 << 24);
    *(UINT32 *)mConfigData.RemoteAddress.Addr = (192 | 168 << 8 | 101 << 16 | 189 << 24);
    mConfigData.RemotePort = mConfigData.StationPort;
    Status = CreateUdp4Socket(&mConfigData, (EFI_EVENT_NOTIFY)Udp4NullHandler, (EFI_EVENT_NOTIFY)Udp4NullHandler, &gSocketTransmit);
    return Status;
}


void lv_efi_app_main(void)
{
  // lv_display_set_theme();

  ui.win_main = lv_win_create(lv_screen_active());
  lv_win_add_title(ui.win_main, "NetAssist");
  ui.header = lv_win_get_header(ui.win_main);
  ui.usage = lv_label_create(ui.header);
  ui.time = lv_label_create(ui.header);
  // lv_obj_align(ui.usage, LV_ALIGN_CENTER, 0, 0);
  ui.usage_timer = lv_timer_create(show_usage, 500, NULL);
  lv_timer_ready(ui.usage_timer);
  ui.btn_cls = lv_win_add_button(ui.win_main, LV_SYMBOL_CLOSE, 45);
  lv_style_init(&ui.win_default);
  lv_style_init(&ui.btn_cls_focused);
  lv_style_init(&ui.btn_cls_clicked);
  lv_style_set_bg_color(&ui.btn_cls_focused, lv_color_make(232, 17, 35));
  lv_style_set_bg_color(&ui.btn_cls_clicked, lv_color_make(241, 112, 122));
  // lv_obj_add_style(ui.win_main, &ui.win_default, LV_STATE_DEFAULT);
  // lv_obj_add_style(ui.btn_cls, &ui.btn_cls_focused, LV_STATE_FOCUSED);
  lv_obj_add_style(ui.btn_cls, &ui.btn_cls_clicked, LV_STATE_PRESSED);
  lv_obj_remove_state(ui.btn_cls, LV_STATE_FOCUSED);
  lv_obj_add_event_cb(ui.btn_cls, event_close, LV_EVENT_RELEASED, NULL);
  ui.content = lv_win_get_content(ui.win_main);
  lv_obj_set_flex_flow(ui.content, LV_FLEX_FLOW_ROW);


  ui.setting.this = lv_obj_create(ui.content);
  static lv_style_t style_title;
  lv_style_init(&style_title);
  lv_style_set_text_color(&style_title, lv_color_hex(0xff0000));
  
  lv_obj_set_size(ui.setting.this, LV_PCT(10), LV_PCT(100));
  lv_obj_align(ui.setting.this, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_border_width(ui.setting.this, 1, 0);
  lv_obj_set_style_border_color(ui.setting.this, lv_color_make(120,120,120), 0);
  // lv_obj_set_style_outline_width(ui.setting.this, 0, 0);
  // lv_obj_set_style_outline_pad(ui.setting.this, 0, 0);
  // lv_obj_set_style_radius(ui.setting.this, 0, 0);
  lv_obj_set_flex_flow(ui.setting.this, LV_FLEX_FLOW_COLUMN);

  ui.setting.net.this = lv_obj_create(ui.setting.this);
  ui.setting.net.lb_title = lv_label_create(ui.setting.net.this);
  ui.setting.net.lb_type = lv_label_create(ui.setting.net.this);
  ui.setting.net.dd_type = lv_dropdown_create(ui.setting.net.this);
  ui.setting.net.lb_ip = lv_label_create(ui.setting.net.this);
  ui.setting.net.ta_ip = lv_textarea_create(ui.setting.net.this);
  ui.setting.net.lb_port = lv_label_create(ui.setting.net.this);
  ui.setting.net.ta_port = lv_textarea_create(ui.setting.net.this);
  ui.setting.net.sw = lv_switch_create(ui.setting.net.this);
  lv_obj_add_event_cb(ui.setting.net.sw, switch_event, LV_EVENT_VALUE_CHANGED, NULL);
  lv_textarea_set_text(ui.setting.net.ta_ip, "192.168.101.17");
  lv_textarea_set_text(ui.setting.net.ta_port, "5566");

  lv_label_set_text(ui.setting.net.lb_title, "Network setting");
  lv_label_set_text(ui.setting.net.lb_type, "Protocol type");
  lv_label_set_text(ui.setting.net.lb_ip, "Station ip");
  lv_label_set_text(ui.setting.net.lb_port, "Station port");
  lv_dropdown_set_options(ui.setting.net.dd_type, "UDP");
  lv_obj_add_style(ui.setting.net.lb_title, &style_title, 0);

  lv_obj_set_style_border_width(ui.setting.net.dd_type, 1, 0);
  lv_obj_set_style_border_color(ui.setting.net.dd_type, lv_color_make(165,165,165), 0);
  lv_obj_set_width(ui.setting.net.dd_type, LV_PCT(50));
  lv_obj_set_style_border_width(ui.setting.net.ta_ip, 1, 0);
  lv_obj_set_style_border_color(ui.setting.net.ta_ip, lv_color_make(165,165,165), 0);
  lv_obj_set_style_border_width(ui.setting.net.ta_port, 1, 0);
  lv_obj_set_style_border_color(ui.setting.net.ta_port, lv_color_make(165,165,165), 0);
  lv_textarea_set_accepted_chars(ui.setting.net.ta_ip, "0123456789.");
  lv_textarea_set_accepted_chars(ui.setting.net.ta_port, "0123456789");
  lv_textarea_set_one_line(ui.setting.net.ta_ip, true);
  lv_textarea_set_one_line(ui.setting.net.ta_port, true);
  lv_textarea_set_max_length(ui.setting.net.ta_ip, 15);
  lv_textarea_set_max_length(ui.setting.net.ta_port, 5);
  lv_obj_set_size(ui.setting.net.ta_ip, LV_PCT(100), 32);
  lv_obj_set_size(ui.setting.net.ta_port, LV_PCT(100), 32);
  lv_obj_set_size(ui.setting.net.sw, LV_PCT(50), 24);

  lv_obj_set_size(ui.setting.net.this, LV_PCT(100), 240);
  lv_obj_align(ui.setting.net.this, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_style_border_width(ui.setting.net.this, 1, 0);
  lv_obj_set_style_border_color(ui.setting.net.this, lv_color_make(165,165,165), 0);
  lv_obj_set_flex_flow(ui.setting.net.this, LV_FLEX_FLOW_COLUMN);

  // lv_obj_align(ui.setting.net.lb_title, LV_ALIGN_OUT_LEFT_TOP, 10, 10);
  // lv_obj_align_to(ui.setting.net.lb_type, ui.setting.net.lb_title, LV_ALIGN_OUT_LEFT_BOTTOM, 20, 0);
  // lv_obj_align_to(ui.setting.net.dd_type, ui.setting.net.lb_type, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
  // lv_obj_align_to(ui.setting.net.lb_ip, ui.setting.net.dd_type, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
  // lv_obj_align_to(ui.setting.net.ta_ip, ui.setting.net.lb_ip, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
  // lv_obj_align_to(ui.setting.net.lb_port, ui.setting.net.ta_ip, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
  // lv_obj_align_to(ui.setting.net.ta_port, ui.setting.net.lb_port, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);
  // lv_obj_align_to(ui.setting.net.sw, ui.setting.net.ta_port, LV_ALIGN_OUT_LEFT_BOTTOM, 0, 0);

  ui.data.this = lv_obj_create(ui.content);
  lv_obj_align_to(ui.data.this, ui.setting.this, LV_ALIGN_OUT_RIGHT_TOP, 0, 0);
  lv_obj_set_style_border_width(ui.data.this, 1, 0);
  lv_obj_set_style_border_color(ui.data.this, lv_color_make(120,120,120), 0);
  lv_obj_set_size(ui.data.this, LV_PCT(90), LV_PCT(100));
  lv_obj_set_flex_flow(ui.data.this, LV_FLEX_FLOW_COLUMN);

  ui.data.lb_data_log = lv_label_create(ui.data.this);
  lv_label_set_text(ui.data.lb_data_log, "Data log");
  lv_obj_add_style(ui.data.lb_data_log, &style_title, 0);
  ui.data.ta_data_log = lv_textarea_create(ui.data.this);
  lv_obj_add_state(ui.data.ta_data_log, LV_STATE_DISABLED);
  lv_obj_set_size(ui.data.ta_data_log, LV_PCT(100), LV_PCT(60));
  lv_obj_set_style_border_width(ui.data.ta_data_log, 1, 0);
  lv_obj_set_style_border_color(ui.data.ta_data_log, lv_color_make(120,120,120), 0);

  ui.data.conf.this = lv_obj_create(ui.data.this);
  lv_obj_align_to(ui.data.conf.this, ui.data.ta_data_log, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
  lv_obj_set_flex_flow(ui.data.conf.this, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(ui.data.conf.this,LV_PCT(100), 25);

  ui.data.conf.lb_send = lv_label_create(ui.data.conf.this);
  lv_label_set_text(ui.data.conf.lb_send, "Data send");
  lv_obj_add_style(ui.data.conf.lb_send, &style_title, 0);

  ui.data.conf.lb_re_ip = lv_label_create(ui.data.conf.this);
  lv_label_set_text(ui.data.conf.lb_re_ip, "  Remote ip:");
  ui.data.conf.ta_re_ip = lv_textarea_create(ui.data.conf.this);
  lv_obj_set_style_border_width(ui.data.conf.ta_re_ip, 1, 0);
  lv_obj_set_style_border_color(ui.data.conf.ta_re_ip, lv_color_make(165,165,165), 0);
  lv_obj_set_size(ui.data.conf.ta_re_ip, 200, LV_PCT(100));
  lv_textarea_set_accepted_chars(ui.data.conf.ta_re_ip, "0123456789.");
  lv_textarea_set_one_line(ui.data.conf.ta_re_ip, true);
  lv_textarea_set_max_length(ui.data.conf.ta_re_ip, 15);
  lv_textarea_set_text(ui.data.conf.ta_re_ip, "192.168.101.189");

  ui.data.conf.lb_re_port = lv_label_create(ui.data.conf.this);
  lv_label_set_text(ui.data.conf.lb_re_port, "  Remote port:");
  ui.data.conf.ta_re_port = lv_textarea_create(ui.data.conf.this);
  lv_obj_set_style_border_width(ui.data.conf.ta_re_port, 1, 0);
  lv_obj_set_style_border_color(ui.data.conf.ta_re_port, lv_color_make(165,165,165), 0);
  lv_obj_set_size(ui.data.conf.ta_re_port, 200, LV_PCT(100));
  lv_textarea_set_accepted_chars(ui.data.conf.ta_re_port, "0123456789");
  lv_textarea_set_one_line(ui.data.conf.ta_re_port, true);
  lv_textarea_set_max_length(ui.data.conf.ta_re_port, 5);
  lv_textarea_set_text(ui.data.conf.ta_re_port, "5566");

  ui.data.conf.btn_clr_send = lv_button_create(ui.data.conf.this);
  lv_obj_add_style(ui.data.conf.btn_clr_send, &ui.btn_cls_clicked, LV_STATE_PRESSED);
  ui.data.conf.lb_clr_send = lv_label_create(ui.data.conf.btn_clr_send);
  lv_label_set_text(ui.data.conf.lb_clr_send, "  clear send");
  ui.data.conf.btn_clr_log = lv_button_create(ui.data.conf.this);
  lv_obj_add_style(ui.data.conf.btn_clr_log, &ui.btn_cls_clicked, LV_STATE_PRESSED);
  ui.data.conf.lb_clr_log = lv_label_create(ui.data.conf.btn_clr_log);
  lv_label_set_text(ui.data.conf.lb_clr_log, "  clear log");

  ui.data.ctrl.this = lv_obj_create(ui.data.this);
  lv_obj_set_flex_flow(ui.data.ctrl.this, LV_FLEX_FLOW_ROW);
  lv_obj_set_size(ui.data.ctrl.this,LV_PCT(100), 340);
  lv_obj_set_style_border_width(ui.data.ctrl.this, 1, 0);
  lv_obj_set_style_border_color(ui.data.ctrl.this, lv_color_make(120,120,120), 0);

  ui.data.ctrl.ta_send = lv_textarea_create(ui.data.ctrl.this);
  lv_obj_set_size(ui.data.ctrl.ta_send, LV_PCT(90), LV_PCT(100));
  lv_obj_set_style_border_width(ui.data.ctrl.ta_send, 1, 0);
  lv_obj_set_style_border_color(ui.data.ctrl.ta_send, lv_color_make(120,120,120), 0);

  ui.data.ctrl.btn_send = lv_button_create(ui.data.ctrl.this);
  lv_obj_add_style(ui.data.ctrl.btn_send, &ui.btn_cls_clicked, LV_STATE_PRESSED);
  lv_obj_add_event_cb(ui.data.ctrl.btn_send, send_event, LV_EVENT_RELEASED, NULL);
  // lv_obj_set_size(ui.data.ctrl.btn_send,)
  ui.data.ctrl.lb_send = lv_label_create(ui.data.ctrl.btn_send);
  lv_label_set_text(ui.data.ctrl.lb_send, "send");
  ui.lb_author = lv_label_create(ui.data.this);
  lv_label_set_text(ui.lb_author, "author: aliothal Yan | address: su zhou");

  UdpInit();
}