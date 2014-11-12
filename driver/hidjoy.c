/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract: This module contains routines Generate the HID report and
    configure the joystick.

Environment:

    Kernel mode


--*/

#include "hidgame.h"

    #pragma alloc_text (PAGE, HGM_DriverInit)
    #pragma alloc_text (PAGE, HGM_GenerateReport)
    #pragma alloc_text (PAGE, HGM_InitAnalog)

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID | HGM_DriverInit |
 *
 *          Perform global initialization.
 *          <nl>This is called from DriverEntry.  Try to initialize a CPU specific
 *          timer but if it fails set up default
 *
 *****************************************************************************/
VOID EXTERNAL
    HGM_DriverInit()
{
/*
    if( !HGM_CPUCounterInit() )
    {
        LARGE_INTEGER QPCFrequency;
        KeQueryPerformanceCounter( &QPCFrequency );
        Global.CounterScale = CALCULATE_SCALE( QPCFrequency.QuadPart );
        Global.ReadCounter = (COUNTER_FUNCTION)&KeQueryPerformanceCounter;

        HGM_DBGPRINT(FILE_HIDJOY | HGM_BABBLE,\
                       ("QPC at %I64u Hz used with scale %d",
                       QPCFrequency.QuadPart, Global.CounterScale ));
    }
*/
}
/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GenerateReport |
 *
 *          Generates a hid report descriptor for a n-axis, m-button joystick,
 *          depending on number of buttons and joy_hws_flags field.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object
 *
 *  @parm   IN OUT UCHAR * | rgGameReport[MAXBYTES_GAME_REPORT] |
 *
 *          Array that receives the HID report descriptor
 *
 *  @parm   OUT PUSHORT | pCbReport |
 *
 *          Address of a short integer that receives size of
 *          HID report descriptor.
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_BUFFER_TOO_SMALL  | Need more memory for HID descriptor
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GenerateReport
    (
    IN PDEVICE_OBJECT       DeviceObject,
    OUT UCHAR               rgGameReport[MAXBYTES_GAME_REPORT],
    OUT PUSHORT             pCbReport
    )
{
    PDEVICE_EXTENSION   DeviceExtension;
    NTSTATUS    ntStatus;
    UCHAR       *pucReport;
    int Idx;
    UCHAR numaxes;

    PAGED_CODE();

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    pucReport = rgGameReport;

#define NEXT_BYTE( pReport, Data )   \
            *pReport++ = Data;

#define NEXT_LONG( pReport, Data )   \
            *(((LONG UNALIGNED*)(pReport))++) = Data;

#define ITEM_DEFAULT        0x00 /* Data, Array, Absolute, No Wrap, Linear, Preferred State, Has no NULL */
#define ITEM_VARIABLE       0x02 /* as ITEM_DEFAULT but value is a variable, not an array */
#define ITEM_HASNULL        0x40 /* as ITEM_DEFAULT but values out of range are considered NULL */
#define ITEM_ANALOG_AXIS    ITEM_VARIABLE
#define ITEM_DIGITAL_POV    (ITEM_VARIABLE|ITEM_HASNULL)
#define ITEM_BUTTON         ITEM_VARIABLE
#define ITEM_PADDING        0x01 /* Constant (nothing else applies) */

      switch(DeviceExtension->psx.type)
      {
        case 0:
        case 1:
        case 2:
          numaxes = 4;
          break;
        case 9:
          numaxes = 3;
          break;
        default:
          numaxes = 2;
      };

    /* USAGE_PAGE (Generic Desktop) */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
    NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);

    /* USAGE (Joystick | GamePad ) */
    NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
    NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_GAMEPAD);

    /* Logical Min is the smallest value that could be produced by a poll */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MIN_1);
    NEXT_BYTE(pucReport,    0 );

    /* Logical Max is the largest value that could be produced by a poll */
    NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_2);
    NEXT_BYTE(pucReport,    255 );
    NEXT_BYTE(pucReport,    0 );

    NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION);
    NEXT_BYTE(pucReport,    0x01 );

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
      NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);

      NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
      NEXT_BYTE(pucReport,    0x01);

      NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION);
      NEXT_BYTE(pucReport,    0x00 );

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_X);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_Y);
        if(numaxes > 2)
        {
          NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
          NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_Z);
          if(numaxes > 3)
          {
            NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
            NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_RZ);
          };
        };
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    8);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    numaxes);

        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_VARIABLE);

      NEXT_BYTE(pucReport,    HIDP_MAIN_ENDCOLLECTION);
      if(numaxes != 4)
      {
        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    0);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    8);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    4-numaxes);

        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);
      };
// Digital Buttons
      NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
      NEXT_BYTE(pucReport,    HID_USAGE_PAGE_BUTTON); // Boton

      NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_MIN_1);
      NEXT_BYTE(pucReport,    1 );

      NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_MAX_1);
      NEXT_BYTE(pucReport,    16 );

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_1);
      NEXT_BYTE(pucReport,    1 );

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
      NEXT_BYTE(pucReport,    0x1);

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
      NEXT_BYTE(pucReport,    16);

      NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
      NEXT_BYTE(pucReport,    ITEM_VARIABLE);
// Analog Buttons
      if(DeviceExtension->psx.Psx2)
      {
      NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_2);
      NEXT_BYTE(pucReport,    255);
      NEXT_BYTE(pucReport,    1);

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
      NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);

      NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION);
      NEXT_BYTE(pucReport,    0x00 );

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_RX);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_RY);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_DIAL);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_GENERIC_SLIDER);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    16);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    4);

        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_VARIABLE);

      NEXT_BYTE(pucReport,    HIDP_MAIN_ENDCOLLECTION);

      } else {
        NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
        NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    0);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    16);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    4);

        NEXT_BYTE(pucReport,    HIDP_MAIN_INPUT_1);
        NEXT_BYTE(pucReport,    ITEM_PADDING);
      };

// FF

      NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
      NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);


      NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION);
      NEXT_BYTE(pucReport,    0x00 );

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    1);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    2);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_2);
        NEXT_BYTE(pucReport,    255 );
        NEXT_BYTE(pucReport,    0 );

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    8);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    2);

        NEXT_BYTE(pucReport,    HIDP_MAIN_OUTPUT_1);
        NEXT_BYTE(pucReport,    ITEM_VARIABLE);

      NEXT_BYTE(pucReport,    HIDP_MAIN_ENDCOLLECTION);

// Info
      NEXT_BYTE(pucReport,    HIDP_GLOBAL_USAGE_PAGE_1);
      NEXT_BYTE(pucReport,    HID_USAGE_PAGE_GENERIC);


      NEXT_BYTE(pucReport,    HIDP_MAIN_COLLECTION);
      NEXT_BYTE(pucReport,    0x00 );

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    1);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    2);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    3);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    4);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    5);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    6);

        NEXT_BYTE(pucReport,    HIDP_LOCAL_USAGE_1);
        NEXT_BYTE(pucReport,    7);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_LOG_MAX_2);
        NEXT_BYTE(pucReport,    255 );
        NEXT_BYTE(pucReport,    0 );

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_SIZE);
        NEXT_BYTE(pucReport,    8);

        NEXT_BYTE(pucReport,    HIDP_GLOBAL_REPORT_COUNT_1);
        NEXT_BYTE(pucReport,    7);

        NEXT_BYTE(pucReport,    HIDP_MAIN_FEATURE_1);
        NEXT_BYTE(pucReport,    ITEM_VARIABLE);

      NEXT_BYTE(pucReport,    HIDP_MAIN_ENDCOLLECTION);

   /* End of collection,  We're done ! */
   NEXT_BYTE(pucReport,  HIDP_MAIN_ENDCOLLECTION);

#undef NEXT_BYTE
#undef NEXT_LONG

    if( pucReport - rgGameReport > MAXBYTES_GAME_REPORT)
    {
        ntStatus   = STATUS_BUFFER_TOO_SMALL;
        *pCbReport = 0x0;
        RtlZeroMemory(rgGameReport, sizeof(rgGameReport));
    } else
    {
        *pCbReport = (USHORT) (pucReport - rgGameReport);
        ntStatus = STATUS_SUCCESS;
    }

    HGM_DBGPRINT( FILE_HIDJOY | HGM_GEN_REPORT,\
                    ("HGM_GenerateReport: ReportSize=0x%x",\
                     *pCbReport) );

    HGM_EXITPROC(FILE_HIDJOY | HGM_FEXIT_STATUSOK, "HGM_GenerateReport", ntStatus);

    return ( ntStatus );
} /* HGM_GenerateReport */

    const int botones[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                             0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000};

NTSTATUS
    HGM_Game2HID
    (
    IN  OUT PUHIDGAME_INPUT_DATA    pHIDData,
    IN PDEVICE_OBJECT       DeviceObject
    )
{
    #define ADD_BUTTON(a, b) ((((a)>0) & (map[(b)] < 16)) ? botones[map[(b)]] : 0 )
    int Id = 1, i;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    HIDGAME_INPUT_DATA  LocalBuffer;
    PDEVICE_EXTENSION   DeviceExtension;
    PUCHAR map;
    PUCHAR finder;
    PUCHAR axismap;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);
    RtlZeroMemory( &LocalBuffer, sizeof( LocalBuffer ) );

    if(DeviceExtension->psx.ScanMode == 0) 
      KeSetEvent(&(DeviceExtension->DoScan), 0, FALSE);
    else
      DeviceExtension->querystill++;

    ntStatus = DeviceExtension->psx.laststatus;
    map = DeviceExtension->psx.buttonmap;

    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.box, 0);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.cross, 1);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.circle, 2);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.triangle, 3);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.l1, 4);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.r1, 5);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.l2, 6);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.r2, 7);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.select, 8);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.start, 9);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.l3, 10);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.r3, 11);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.lf, 12);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.dn, 13);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.rt, 14);
    LocalBuffer.Buttons |= ADD_BUTTON(DeviceExtension->psx.up, 15);

    if(DeviceExtension->psx.Psx2) {
      finder = (PUCHAR) &(DeviceExtension->psx);
      for(i=0; i<16; i++)
      {
        switch(map[i])
        {
             case 16:
               LocalBuffer.AnalogButtons[0] = ((-1) * (256+finder[i]))+511;
               break;
             case 17:
               LocalBuffer.AnalogButtons[1] = ((-1) * (256+finder[i]))+511;
               break;
             case 18:
               LocalBuffer.AnalogButtons[2] = ((-1) * (256+finder[i]))+511;
               break;
             case 19:
               LocalBuffer.AnalogButtons[3] = ((-1) * (256+finder[i]))+511;
               break;
        }
      }
    };

    axismap = (PUCHAR) &DeviceExtension->psx.Ejes;

    LocalBuffer.Axis[axismap[0]] = DeviceExtension->psx.lx;
    LocalBuffer.Axis[axismap[1]] = DeviceExtension->psx.ly;
    LocalBuffer.Axis[axismap[2]] = DeviceExtension->psx.ry;
    LocalBuffer.Axis[axismap[3]] = DeviceExtension->psx.rx;

    for(i=0;i<4; i++)
      if((LocalBuffer.Axis[i] >= DeviceExtension->psx.min) &&
           (LocalBuffer.Axis[i] <= DeviceExtension->psx.max))
         LocalBuffer.Axis[i] = 127;
    C_ASSERT( sizeof( *pHIDData ) == sizeof( LocalBuffer ) );
    RtlCopyMemory( pHIDData, &LocalBuffer, sizeof( LocalBuffer ) );

    return ntStatus;

} /* HGM_Game2HID */

