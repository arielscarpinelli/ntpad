/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    ioctl.c

Abstract: Contains routines to support HIDCLASS internal
          ioctl queries for game devices.

Environment:

    Kernel mode


--*/

#include "hidgame.h"
#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_GetDeviceDescriptor)
    #pragma alloc_text (PAGE, HGM_GetReportDescriptor)
    #pragma alloc_text (PAGE, HGM_GetAttributes      )
#endif
/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_InternalIoctl |
 *
 *          Process the Control IRPs sent to this device.
 *          <nl>This function cannot be pageable because reads/writes
 *          can be made at dispatch-level
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_NOT_SUPPORT | Irp function not supported
 *  @rvalue   ???            | ???
 *
 *****************************************************************************/
NTSTATUS EXTERNAL
    HGM_InternalIoctl
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;

    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,   \
                   ("HGM_InternalIoctl(DeviceObject=0x%x,Irp=0x%x)", \
                    DeviceObject, Irp));

    /*
     *  Get a pointer to the current location in the Irp
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     *  Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);


    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         *  Someone sent us another plug and play IRP after removed
         */

        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_InternalIoctl: PnP IRP after device was removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
    } else
    {
        switch(IrpStack->Parameters.DeviceIoControl.IoControlCode)
        {
                
            case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE, \
                               ("IOCTL_HID_GET_DEVICE_DESCRIPTOR"));
                ntStatus = HGM_GetDeviceDescriptor(DeviceObject, Irp);
                break;

            case IOCTL_HID_GET_REPORT_DESCRIPTOR:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE, \
                               ("IOCTL_HID_GET_REPORT_DESCRIPTOR"));
                ntStatus = HGM_GetReportDescriptor(DeviceObject, Irp);
                break;

            case IOCTL_HID_READ_REPORT:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE,\
                               ("IOCTL_HID_READ_REPORT"));
                ntStatus = HGM_ReadReport(DeviceObject, Irp);
                break;

            case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE,\
                               ("IOCTL_HID_GET_DEVICE_ATTRIBUTES"));
                ntStatus = HGM_GetAttributes(DeviceObject, Irp);
                break;

            case IOCTL_HID_GET_FEATURE:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE,\
                               ("IOCTL_HID_GET_FEATURE_REPORT"));
                ntStatus = HGM_GetFeature(DeviceObject, Irp);
                break;

            case IOCTL_HID_WRITE_REPORT:
                HGM_DBGPRINT(FILE_IOCTL | HGM_BABBLE,\
                               ("IOCTL_HID_WRITE_REPORT"));
                ntStatus = HGM_WriteReport(DeviceObject, Irp);
                break;

            default:
                ntStatus = STATUS_NOT_SUPPORTED;
                break;
        }


        /*
         * Set real return status in Irp
         */
        Irp->IoStatus.Status = ntStatus;

        HGM_DecRequestCount( DeviceExtension );
    }


    if(ntStatus != STATUS_PENDING)
    {
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        /*
         * NOTE: Real return status set in Irp->IoStatus.Status
         */
        ntStatus = STATUS_SUCCESS;
    } else
    {
        /*
         * No reason why there should be a status pending
         */
        HGM_DBGPRINT(FILE_IOCTL | HGM_ERROR, \
                       ("HGM_InternalIoctl: Pending Status !"));
        IoMarkIrpPending( Irp );
    }

    HGM_EXITPROC(FILE_IOCTL | HGM_FEXIT_STATUSOK, "HGM_InternalIoctl", ntStatus);

    return ntStatus;
} /* HGM_InternalIoctl */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GetDeviceDescriptor |
 *
 *          Respond to HIDCLASS IOCTL_HID_GET_DEVICE_DESCRIPTOR
 *          by returning a device descriptor
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_BUFFER_TOO_SMALL |  need more memory
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GetDeviceDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PHID_DESCRIPTOR pHidDescriptor;        /* Hid descriptor for this device */
    USHORT   cbReport;
    UCHAR               rgGameReport[MAXBYTES_GAME_REPORT] ;
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,\
                   ("HGM_GetDeviceDescriptor(DeviceObject=0x%x,Irp=0x%x)",
                    DeviceObject, Irp));

    /*
     * Get a pointer to the current location in the Irp
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     *  Get a pointer to the HID_DESCRIPTOR
     */
    pHidDescriptor =  (PHID_DESCRIPTOR) Irp->UserBuffer;


    if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*pHidDescriptor)  )
    {

        HGM_DBGPRINT(FILE_IOCTL | HGM_ERROR,\
                       ("HGM_GetDeviceDescriptor: OutBufferLength(0x%x) < sizeof(HID_DESCRIPTOR)(0x%x)", \
                        IrpStack->Parameters.DeviceIoControl.OutputBufferLength, sizeof(*pHidDescriptor)));


        ntStatus = STATUS_BUFFER_TOO_SMALL;
    } else
    {
        /*
         * Generate the report
         */
        ntStatus =  HGM_GenerateReport(DeviceObject, rgGameReport, &cbReport);

        if( NT_SUCCESS(ntStatus) )
        {
            RtlZeroMemory( pHidDescriptor, sizeof(*pHidDescriptor) );
            /*
             * Copy device descriptor to HIDCLASS buffer
             */
            pHidDescriptor->bLength                         = sizeof(*pHidDescriptor);
            pHidDescriptor->bDescriptorType                 = HID_HID_DESCRIPTOR_TYPE;
            pHidDescriptor->bcdHID                          = HID_REVISION;
            pHidDescriptor->bCountry                        = 0; /*not localized*/
            pHidDescriptor->bNumDescriptors                 = HGM_NUMBER_DESCRIPTORS;
            pHidDescriptor->DescriptorList[0].bReportType   = HID_REPORT_DESCRIPTOR_TYPE ;
            pHidDescriptor->DescriptorList[0].wReportLength = cbReport;

            /*
             * Report how many bytes were copied
             */
            Irp->IoStatus.Information = sizeof(*pHidDescriptor);
        } else
        {
            Irp->IoStatus.Information = 0x0;
        }
    }

    HGM_EXITPROC(FILE_IOCTL |HGM_FEXIT_STATUSOK, "HGM_GetDeviceDescriptor", ntStatus);

    return ntStatus;
} /* HGM_GetDeviceDescriptor */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GetReportDescriptor |
 *
 *          Respond to HIDCLASS IOCTL_HID_GET_REPORT_DESCRIPTOR
 *          by returning appropriate the report descriptor
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | ???
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GetReportDescriptor
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION     DeviceExtension;
    PIO_STACK_LOCATION    IrpStack;
    NTSTATUS              ntStatus;
    UCHAR                 rgGameReport[MAXBYTES_GAME_REPORT] ;
    USHORT                cbReport;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,\
                   ("HGM_GetReportDescriptor(DeviceObject=0x%x,Irp=0x%x)",\
                    DeviceObject, Irp));

    /*
     * Get a pointer to the current location in the Irp
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * Get a pointer to the device extension
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);


    /*
     * Generate the report
     */
    ntStatus =  HGM_GenerateReport(DeviceObject, rgGameReport, &cbReport);

    if( NT_SUCCESS(ntStatus) )
    {
        if( cbReport >  (USHORT) IrpStack->Parameters.DeviceIoControl.OutputBufferLength )
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;

            HGM_DBGPRINT(FILE_IOCTL | HGM_ERROR,\
                           ("HGM_GetReportDescriptor: cbReport(0x%x) OutputBufferLength(0x%x)",\
                            cbReport, IrpStack->Parameters.DeviceIoControl.OutputBufferLength));

        } else
        {
            RtlCopyMemory( Irp->UserBuffer, rgGameReport, cbReport );
            /*
             * Report how many bytes were copied
             */
            Irp->IoStatus.Information = cbReport;
            ntStatus = STATUS_SUCCESS;
        }
    }

    HGM_EXITPROC(FILE_IOCTL |HGM_FEXIT_STATUSOK, "HGM_GetReportDescriptor", ntStatus);

    return ntStatus;
} /* HGM_GetReportDescriptor */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_ReadReport |
 *
 *          Poll the gameport, remap the axis and button data and package
 *          into the defined HID report field.
 *          <nl>This routine cannot be pageable as HID can make reads at 
 *          dispatch-level.
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS  | success
 *  @rvalue   STATUS_DEVICE_NOT_CONNECTED | Device Failed to Quiesce 
 *                                          ( not connected )
 *  @rvalue   STATUS_TIMEOUT  | Could not determine exact transition time for 
 *                              one or more axis but not a failure.
 *
 *****************************************************************************/
NTSTATUS  INTERNAL
    HGM_ReadReport
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;

    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,\
                   ("HGM_ReadReport(DeviceObject=0x%x,Irp=0x%x)", \
                    DeviceObject, Irp));

    /*
     * Get a pointer to the device extension.
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Get Stack location.
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * First check the size of the output buffer (there is no input buffer)
     */
    if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(HIDGAME_INPUT_DATA) )
    {
        HGM_DBGPRINT(FILE_IOCTL | HGM_WARN,\
                       ("HGM_ReadReport: Buffer too small, output=0x%x need=0x%x", \
                        IrpStack->Parameters.DeviceIoControl.OutputBufferLength, 
                        sizeof(HIDGAME_INPUT_DATA) ) );

        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    if( DeviceExtension->fStarted == FALSE )
    {
        ntStatus = STATUS_DEVICE_NOT_READY ;
    }

    /*
     *  All the checking done so do device specific polling
     */
    if( NT_SUCCESS(ntStatus) )
    {
        ntStatus = HGM_Game2HID( (PHIDGAME_INPUT_DATA)Irp->UserBuffer,
                                 DeviceObject );
        Irp->IoStatus.Information = sizeof(HIDGAME_INPUT_DATA);
    } 
    else
    {
        Irp->IoStatus.Information = 0x0;
    }

    Irp->IoStatus.Status = ntStatus;


    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT,  "HGM_ReadReport", ntStatus);

    return ntStatus;
} /* HGM_ReadReport */



/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_GetAttributes |
 *
 *          Respond to IOCTL_HID_GET_ATTRIBUTES, by filling
 *          the HID_DEVICE_ATTRIBUTES struct
 *
 *  @parm   IN PDRIVER_OBJECT | DeviceObject |
 *
 *          Pointer to the driver object
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O Request Packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | ???
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_GetAttributes
    (
    PDEVICE_OBJECT  DeviceObject,
    PIRP            Irp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PIO_STACK_LOCATION  IrpStack;

    PAGED_CODE();

    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,\
                   ("HGM_GetAttributes(DeviceObject=0x%x,Irp=0x%x)",\
                    DeviceObject, Irp));

    /*
     * Get a pointer to the current location in the Irp
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    if( IrpStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof (HID_DEVICE_ATTRIBUTES)   )
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;

        HGM_DBGPRINT(FILE_IOCTL | HGM_ERROR,\
                       ("HGM_GetAttributes: cbReport(0x%x) OutputBufferLength(0x%x)",\
                        sizeof (HID_DEVICE_ATTRIBUTES), IrpStack->Parameters.DeviceIoControl.OutputBufferLength));
    } else
    {
        PHID_DEVICE_ATTRIBUTES  DeviceAttributes;

        DeviceAttributes = (PHID_DEVICE_ATTRIBUTES) Irp->UserBuffer;


        RtlZeroMemory( DeviceAttributes, sizeof(*DeviceAttributes));

        /*
         * Report how many bytes were copied
         */

        Irp->IoStatus.Information   = sizeof(*DeviceAttributes);

        DeviceAttributes->Size          = sizeof (*DeviceAttributes);
        DeviceAttributes->VendorID      = 0x045E;
        DeviceAttributes->ProductID     = 0X2222;
        DeviceAttributes->VersionNumber = HIDGAME_VERSION_NUMBER;

    }

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT_STATUSOK, "HGM_GetAttributes", ntStatus);

    return ntStatus;
} /* HGM_GetAttributes */
// *****************************************************************************
NTSTATUS  INTERNAL
    HGM_WriteReport
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;
    PUCHAR ReportBuffer;

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Get Stack location.
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * First check the size of the output buffer (there is no input buffer)
     */

    if( DeviceExtension->fStarted == FALSE )
    {
        ntStatus = STATUS_DEVICE_NOT_READY ;
    }

    /*
     *  All the checking done so do device specific polling
     */
    ReportBuffer = ((HID_XFER_PACKET*)Irp->UserBuffer)->reportBuffer;
    if( NT_SUCCESS(ntStatus) )
    {
        if(ReportBuffer[0] == 255)
        {
           Irp->IoStatus.Status = HGM_InitDevice(DeviceObject, Irp);
           return(Irp->IoStatus.Status);
        }
        if(ReportBuffer[0] == 2)
        {
           DeviceExtension->fetscan = 1;
           return(Irp->IoStatus.Status);
        };
        if(!DeviceExtension->psx.noFF)
        {
          DeviceExtension->psx.motorg = ReportBuffer[1];
          if(ReportBuffer[0] == 1)
            DeviceExtension->psx.motorc = 1;
          else
            DeviceExtension->psx.motorc = 0;
          DeviceExtension->querystill++;
        };
        if(DeviceExtension->querystill > 1000)
          KeSetEvent(&(DeviceExtension->DoScan), 0, FALSE);
        Irp->IoStatus.Information = sizeof(HID_XFER_PACKET);
    }
    else
    {
        Irp->IoStatus.Information = 0x0;
    }

    Irp->IoStatus.Status = ntStatus;

    return ntStatus;
} /* HGM_WriteReport */
// *****************************************************************************
NTSTATUS  INTERNAL
    HGM_GetFeature
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    PDEVICE_EXTENSION   DeviceExtension;
    PIO_STACK_LOCATION  IrpStack;
    extern const int botones[];
    int prev;
    HGM_DBGPRINT(FILE_IOCTL | HGM_FENTRY,\
                   ("HGM_ReadReport(DeviceObject=0x%x,Irp=0x%x)", \
                    DeviceObject, Irp));

    /*
     * Get a pointer to the device extension.
     */

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Get Stack location.
     */

    IrpStack = IoGetCurrentIrpStackLocation(Irp);

    /*
     * First check the size of the output buffer (there is no input buffer)
     */
    if( ((HID_XFER_PACKET*)Irp->UserBuffer)->reportBufferLen < 7 )
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
    }

    if( DeviceExtension->fStarted == FALSE )
    {
        ntStatus = STATUS_DEVICE_NOT_READY ;
    }

    /*
     *  All the checking done so do device specific polling
     */
    if( NT_SUCCESS(ntStatus) )
    {
        PUCHAR ReportBuffer;
        unsigned short *Buttons;
        ReportBuffer = ((HID_XFER_PACKET*)Irp->UserBuffer)->reportBuffer;
        if(DeviceExtension->fetscan)
        {
          KeClearEvent(&(DeviceExtension->ScanReady));
          prev = DeviceExtension->psx.noAx;
          DeviceExtension->psx.noAx = 1;
          KeSetEvent(&(DeviceExtension->DoScan), 0, FALSE);
          KeWaitForSingleObject(&(DeviceExtension->ScanReady), UserRequest, KernelMode, FALSE, 0);
          DeviceExtension->psx.noAx = prev;
        };
        DeviceExtension->fetscan = 0;
        Buttons = (unsigned short*)ReportBuffer;
        #define ADD_BUTTON(a, b) (((a)>0) ? botones[(b)] : 0)
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.box, 0);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.cross, 1);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.circle, 2);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.triangle, 3);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.l1, 4);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.r1, 5);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.l2, 6);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.r2, 7);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.select, 8);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.start, 9);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.l3, 10);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.r3, 11);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.lf, 12);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.dn, 13);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.rt, 14);
        *Buttons |= ADD_BUTTON(DeviceExtension->psx.up, 15);
        ReportBuffer[2] = (UCHAR)DeviceExtension->DeviceID;
        ReportBuffer[3] = DeviceExtension->psx.lx;
        ReportBuffer[4] = DeviceExtension->psx.ly;
        Irp->IoStatus.Information = 8;
    } 
    else
    {
        Irp->IoStatus.Information = 0x0;
    }

    Irp->IoStatus.Status = ntStatus;


    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT,  "HGM_ReadReport", ntStatus);

    return ntStatus;
} /* HGM_ReadReport */

// *****************************************************************************
NTSTATUS INTERNAL
    HGM_ActivateDevice
    (
    PDEVICE_OBJECT  DeviceObject
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    PDEVICE_EXTENSION   DeviceExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Thread;
    PVOID pkThread;
    PAGED_CODE();
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    if(!DeviceExtension->ScanThread)
//    if(!Global.ThreadIniciado)
    {
//      Global.ThreadIniciado = 1;
      InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL)
      KeInitializeEvent(&(DeviceExtension->FinishThread), NotificationEvent, FALSE);
      KeInitializeEvent(&(DeviceExtension->DoScan), SynchronizationEvent, FALSE);
      KeInitializeEvent(&(DeviceExtension->ScanReady), SynchronizationEvent, FALSE);
      ntStatus = PsCreateSystemThread(&Thread, THREAD_ALL_ACCESS,
                                       &ObjectAttributes, NULL, NULL, ScaningThread, (PVOID) DeviceObject);
      ObReferenceObjectByHandle(Thread, THREAD_ALL_ACCESS, NULL, KernelMode, &pkThread,NULL);
      KeSetPriorityThread(pkThread, LOW_REALTIME_PRIORITY - 1);
      DeviceExtension->ScanThread = (PKTHREAD) pkThread;
    };
    return ntStatus;
} /* HGM_GetAttributes */


