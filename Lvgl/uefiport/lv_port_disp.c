/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/
static EFI_GRAPHICS_OUTPUT_PROTOCOL *Gop = NULL;
/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Example 1
     * One buffer for partial rendering*/
    // static lv_color_t buf_1_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
    // lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * 10];
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * 10];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(disp);
    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
    // static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];
    // static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];
    // lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);

}

void lv_port_disp_deinit(void)
{
    lv_display_delete(lv_display_get_default());
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
    gBS->HandleProtocol(gST->ConsoleOutHandle, &gEfiGraphicsOutputProtocolGuid, (void **)&Gop);
    if (Gop == NULL)
        gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void **)&Gop);
    UINTN       SizeOfInfo = 0;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *Info = NULL;
    UINT32 i = 0;
    UINTN Columns = 0, Rows = 0;
    for (i = 0; i < gST->ConOut->Mode->MaxMode; i++) {
        gST->ConOut->QueryMode(gST->ConOut, i, &Columns, &Rows);
        if (Columns == 240 && Rows == 56) {
            gST->ConOut->SetMode(gST->ConOut, i);
            break;
        }
    }
    for (i = 0; i < Gop->Mode->MaxMode; i++) {
        Gop->QueryMode(Gop, i, &SizeOfInfo, &Info);
        if ((Info->HorizontalResolution == MY_DISP_HOR_RES) && (Info->VerticalResolution == MY_DISP_VER_RES)) {
            Gop->SetMode(Gop, i);
            break;
        }
    }
    if (i == Gop->Mode->MaxMode) {
        AsciiPrint("%d * %d is not support!!!\n", MY_DISP_HOR_RES, MY_DISP_VER_RES);
        gBS->Stall(10000000);
        lv_efi_app_exit();
    }
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
    if(disp_flush_enabled) {
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/

        // int32_t x;
        // int32_t y;
        // for(y = area->y1; y <= area->y2; y++) {
        //     for(x = area->x1; x <= area->x2; x++) {
        //         /*Put a pixel to the display. For example:*/
        //         /*put_px(x, y, *px_map)*/
        //         px_map++;
        //     }
        // }
        Gop->Blt(Gop, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)px_map, EfiBltBufferToVideo,
                0, 0, area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1), 0);
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
