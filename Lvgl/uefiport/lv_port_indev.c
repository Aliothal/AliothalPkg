/**
 * @file lv_port_indev_templ.c
 *
 */

/*Copy this file as "lv_port_indev.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_indev.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void touchpad_init(void);
static void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool touchpad_is_pressed(void);
static void touchpad_get_xy(int32_t * x, int32_t * y);

static void mouse_init(void);
static void mouse_read(lv_indev_t * indev, lv_indev_data_t * data);
static bool mouse_is_pressed(void);
static void mouse_get_xy(int32_t * x, int32_t * y);

static void keypad_init(void);
static void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
static uint32_t keypad_get_key(void);

static void encoder_init(void);
static void encoder_read(lv_indev_t * indev, lv_indev_data_t * data);
static void encoder_handler(void);

static void button_init(void);
static void button_read(lv_indev_t * indev, lv_indev_data_t * data);
static int8_t button_get_pressed_id(void);
static bool button_is_pressed(uint8_t id);

/**********************
 *  STATIC VARIABLES
 **********************/
lv_indev_t * indev_touchpad;
lv_indev_t * indev_mouse;
lv_indev_t * indev_keypad;
lv_indev_t * indev_encoder;
lv_indev_t * indev_button;
lv_group_t * indev_group;

static int32_t encoder_diff;
static lv_indev_state_t encoder_state;

static EFI_SIMPLE_POINTER_PROTOCOL *Spp = NULL;
static EFI_SIMPLE_POINTER_STATE SpState;
static UINT8 ScaleX = 0, ScaleY = 0;
static INT32 PositionX = (MY_DISP_HOR_RES >> 1);
static INT32 PositionY = (MY_DISP_VER_RES >> 1);

static EFI_SIMPLE_TEXT_INPUT_PROTOCOL *Stip = NULL;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_indev_init(void)
{
    /**
     * Here you will find example implementation of input devices supported by LittelvGL:
     *  - Touchpad
     *  - Mouse (with cursor support)
     *  - Keypad (supports GUI usage only with key)
     *  - Encoder (supports GUI usage only with: left, right, push)
     *  - Button (external buttons to press points on the screen)
     *
     *  The `..._read()` function are only examples.
     *  You should shape them according to your hardware
     */

    indev_group = lv_group_create();
    lv_group_set_default(indev_group);

    /*------------------
     * Touchpad
     * -----------------*/

    // /*Initialize your touchpad if you have*/
    // touchpad_init();

    // /*Register a touchpad input device*/
    // indev_touchpad = lv_indev_create();
    // lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    // lv_indev_set_read_cb(indev_touchpad, touchpad_read);

    /*------------------
     * Mouse
     * -----------------*/

    /*Initialize your mouse if you have*/
    mouse_init();

    /*Register a mouse input device*/
    indev_mouse = lv_indev_create();
    lv_indev_set_type(indev_mouse, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_mouse, mouse_read);

    /*Set cursor. For simplicity set a HOME symbol now.*/
    lv_obj_t * mouse_cursor = lv_image_create(lv_screen_active());
    lv_image_set_src(mouse_cursor, LV_SYMBOL_HOME);
    // lv_image_set_src(mouse_cursor, "S:/normal.png");  //Degrade performance
    lv_indev_set_cursor(indev_mouse, mouse_cursor);
    lv_indev_set_group(indev_mouse, indev_group);

    /*------------------
     * Keypad
     * -----------------*/

    /*Initialize your keypad or keyboard if you have*/
    keypad_init();

    /*Register a keypad input device*/
    indev_keypad = lv_indev_create();
    lv_indev_set_type(indev_keypad, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev_keypad, keypad_read);
    lv_indev_set_group(indev_keypad, indev_group);
    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_keypad, group);`*/

    /*------------------
     * Encoder
     * -----------------*/

    // /*Initialize your encoder if you have*/
    // encoder_init();

    // /*Register a encoder input device*/
    // indev_encoder = lv_indev_create();
    // lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    // lv_indev_set_read_cb(indev_touchpad, encoder_read);

    /*Later you should create group(s) with `lv_group_t * group = lv_group_create()`,
     *add objects to the group with `lv_group_add_obj(group, obj)`
     *and assign this input device to group to navigate in it:
     *`lv_indev_set_group(indev_encoder, group);`*/

    /*------------------
     * Button
     * -----------------*/

    // /*Initialize your button if you have*/
    // button_init();

    // /*Register a button input device*/
    // indev_button = lv_indev_create();
    // lv_indev_set_type(indev_button, LV_INDEV_TYPE_BUTTON);
    // lv_indev_set_read_cb(indev_button, button_read);

    // /*Assign buttons to points on the screen*/
    // static const lv_point_t btn_points[2] = {
    //     {10, 10},   /*Button 0 -> x:10; y:10*/
    //     {40, 100},  /*Button 1 -> x:40; y:100*/
    // };
    // lv_indev_set_button_points(indev_button, btn_points);
}

void lv_port_indev_deinit(void)
{
    lv_indev_delete(indev_keypad);
    lv_indev_delete(indev_mouse);
    lv_group_delete(indev_group);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*------------------
 * Touchpad
 * -----------------*/

/*Initialize your touchpad*/
static void touchpad_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the touchpad*/
static void touchpad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static int32_t last_x = 0;
    static int32_t last_y = 0;

    /*Save the pressed coordinates and the state*/
    if(touchpad_is_pressed()) {
        touchpad_get_xy(&last_x, &last_y);
        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    /*Set the last pressed coordinates*/
    data->point.x = last_x;
    data->point.y = last_y;
}

/*Return true is the touchpad is pressed*/
static bool touchpad_is_pressed(void)
{
    /*Your code comes here*/

    return false;
}

/*Get the x and y coordinates if the touchpad is pressed*/
static void touchpad_get_xy(int32_t * x, int32_t * y)
{
    /*Your code comes here*/

    (*x) = 0;
    (*y) = 0;
}

/*------------------
 * Mouse
 * -----------------*/

/*Initialize your mouse*/
static void mouse_init(void)
{
    gBS->HandleProtocol(gST->ConsoleInHandle, &gEfiSimplePointerProtocolGuid, &Spp);
    UINT64 ResolutionX = Spp->Mode->ResolutionX;
    UINT64 ResolutionY = Spp->Mode->ResolutionY;

    while (ResolutionX != 1) {
        ResolutionX >>= 1;
        ScaleX++;
    }
    while (ResolutionY != 1) {
        ResolutionY >>= 1;
        ScaleY++;
    }
}

/*Will be called by the library to read the mouse*/
static void mouse_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    /*Get the current x and y coordinates*/
    mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the mouse button is pressed or released*/
    if(mouse_is_pressed()) {
        data->state = LV_INDEV_STATE_PR;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }
}

/*Return true is the mouse button is pressed*/
static bool mouse_is_pressed(void)
{
    if (SpState.LeftButton && SpState.RightButton)
        lv_efi_app_exit();
    return SpState.LeftButton | SpState.RightButton;
}

/*Get the x and y coordinates if the mouse is pressed*/
static void mouse_get_xy(int32_t * x, int32_t * y)
{
    // if (gBS->CheckEvent(Spp->WaitForInput)==0) {      //Degrade performance
    //     Spp->GetState(Spp, &SpState);
    EFI_SIMPLE_POINTER_STATE CurSpState;
    if (Spp->GetState(Spp, &CurSpState) == 0) {
        SpState = CurSpState;
        PositionX += (SpState.RelativeMovementX >> (ScaleX - 3));
        PositionY += (SpState.RelativeMovementY >> (ScaleY - 3));
        if (PositionX > (MY_DISP_HOR_RES - 1))
            PositionX = (MY_DISP_HOR_RES - 1);
        else if (PositionX < 0)
            PositionX = 0;
        if (PositionY > (MY_DISP_VER_RES - 1))
            PositionY = (MY_DISP_VER_RES - 1);
        else if (PositionY < 0)
            PositionY = 0;
    }
    *x = PositionX;
    *y = PositionY;
}

/*------------------
 * Keypad
 * -----------------*/

/*Initialize your keypad*/
static void keypad_init(void)
{
    Stip = gST->ConIn;
}

/*Will be called by the library to read the mouse*/
static void keypad_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{
    static uint32_t last_key = 0;

    /*Get the current x and y coordinates*/
    // mouse_get_xy(&data->point.x, &data->point.y);

    /*Get whether the a key is pressed and save the pressed key*/
    uint32_t act_key = keypad_get_key();
    if(act_key != 0) {
        data->state = LV_INDEV_STATE_PR;
        last_key = act_key;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    data->key = last_key;
}

/*Get the currently being pressed key.  0 if no key is pressed*/
static uint32_t keypad_get_key(void)
{
    EFI_INPUT_KEY Key;

    if (Stip->ReadKeyStroke(Stip, &Key) == 0) {
        switch(Key.ScanCode) {
            case SCAN_UP:
                return LV_KEY_UP;
            case SCAN_DOWN:
                return LV_KEY_DOWN;
            case SCAN_RIGHT:
                return LV_KEY_RIGHT;
            case SCAN_LEFT:
                return LV_KEY_LEFT;
            case SCAN_HOME:
                return LV_KEY_HOME;
            case SCAN_END:
                return LV_KEY_END;
            case SCAN_DELETE:
                return LV_KEY_DEL;
            case SCAN_PAGE_UP:
                return LV_KEY_PREV;
            case SCAN_PAGE_DOWN:
                return LV_KEY_NEXT;
            case SCAN_ESC:
                return LV_KEY_ESC;
            case SCAN_NULL:
                switch(Key.UnicodeChar) {
                    case CHAR_CARRIAGE_RETURN:
                        return LV_KEY_ENTER;
                    default:
                        return Key.UnicodeChar;
                }
            default:
                return Key.UnicodeChar;
        }
    }

    return 0;
}

/*------------------
 * Encoder
 * -----------------*/

/*Initialize your encoder*/
static void encoder_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the encoder*/
static void encoder_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    data->enc_diff = encoder_diff;
    data->state = encoder_state;
}

/*Call this function in an interrupt to process encoder events (turn, press)*/
static void encoder_handler(void)
{
    /*Your code comes here*/

    encoder_diff += 0;
    encoder_state = LV_INDEV_STATE_REL;
}

/*------------------
 * Button
 * -----------------*/

/*Initialize your buttons*/
static void button_init(void)
{
    /*Your code comes here*/
}

/*Will be called by the library to read the button*/
static void button_read(lv_indev_t * indev_drv, lv_indev_data_t * data)
{

    static uint8_t last_btn = 0;

    /*Get the pressed button's ID*/
    int8_t btn_act = button_get_pressed_id();

    if(btn_act >= 0) {
        data->state = LV_INDEV_STATE_PR;
        last_btn = btn_act;
    }
    else {
        data->state = LV_INDEV_STATE_REL;
    }

    /*Save the last pressed button's ID*/
    data->btn_id = last_btn;
}

/*Get ID  (0, 1, 2 ..) of the pressed button*/
static int8_t button_get_pressed_id(void)
{
    uint8_t i;

    /*Check to buttons see which is being pressed (assume there are 2 buttons)*/
    for(i = 0; i < 2; i++) {
        /*Return the pressed button's ID*/
        if(button_is_pressed(i)) {
            return i;
        }
    }

    /*No button pressed*/
    return -1;
}

/*Test if `id` button is pressed or not*/
static bool button_is_pressed(uint8_t id)
{

    /*Your code comes here*/

    return false;
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif