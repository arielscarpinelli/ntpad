
/*++

Copyright (c) 1998 - 1999  Microsoft Corporation

Module Name:

    pnp.c

Abstract: This module contains PnP Start, Stop, Remove,
          Power dispatch routines and IRP cancel routine.

Environment:

    Kernel mode

--*/

#include "hidgame.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (PAGE, HGM_RemoveDevice)
    #pragma alloc_text (PAGE, HGM_PnP)
    #pragma alloc_text (PAGE, HGM_InitDevice)
    #pragma alloc_text (PAGE, HGM_GetResources)
    #pragma alloc_text (PAGE, HGM_Power)
    #pragma alloc_text (PAGE, HGM_PnPComplete)
#endif


/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   NTSTATUS  | HGM_IncRequestCount |
 *
 *          Try to increment the request count but fail if the device is 
 *          being removed.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *  @rvalue STATUS_SUCCESS | success
 *  @rvalue STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *
 *****************************************************************************/
NTSTATUS  EXTERNAL
    HGM_IncRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    NTSTATUS    ntStatus;

    InterlockedIncrement( &DeviceExtension->RequestCount );
    ASSERT( DeviceExtension->RequestCount > 0 );
    
    if( DeviceExtension->fRemoved )
    {
        /*
         *  PnP has already told us to remove the device so fail and make 
         *  sure that the event has been set.
         */
        if( 0 == InterlockedDecrement( &DeviceExtension->RequestCount ) ) 
        {
            KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
        }
        ntStatus = STATUS_DELETE_PENDING;
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
    



/*****************************************************************************
 *
 *  @doc    INTERNAL
 *
 *  @func   VOID  | HGM_DecRequestCount |
 *
 *          Decrement the request count and set event if this is the last.
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID EXTERNAL
    HGM_DecRequestCount
    (
    IN PDEVICE_EXTENSION DeviceExtension
    )
{
    LONG        LocalCount;

    LocalCount = InterlockedDecrement( &DeviceExtension->RequestCount );

    ASSERT( DeviceExtension->RequestCount >= 0 );
    
    if( LocalCount == 0 )
    {
        /*
         *  PnP has already told us to remove the device so the PnP remove 
         *  code should have set device as removed and should be waiting on
         *  the event.
         */
        ASSERT( DeviceExtension->fRemoved );
        KeSetEvent( &DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE );
    }

    return;
}
    

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   VOID  | HGM_RemoveDevice |
 *
 *          FDO Remove routine 
 *
 *  @parm   IN PDEVICE_EXTENSION | DeviceExtension |
 *
 *          Pointer to the device extension.
 *
 *****************************************************************************/
VOID INTERNAL
    HGM_RemoveDevice
    (
    PDEVICE_EXTENSION DeviceExtension
    )
{
    if (DeviceExtension->fSurpriseRemoved) {
        return;
    }

    DeviceExtension->fSurpriseRemoved = TRUE;

    /*
     *  Acquire mutex before modifying the Global Linked list of devices
     */
    ExAcquireFastMutex (&Global.Mutex);
    
    /*
     * Remove this device from the linked list of devices
     */
    RemoveEntryList(&DeviceExtension->Link);
    
    /*
     *  Release the mutex
     */
    ExReleaseFastMutex (&Global.Mutex);
/*
    if(DeviceExtension->ScanThread)
    {
*/
      KeSetEvent(&(DeviceExtension->FinishThread), 0, TRUE);
      KeWaitForSingleObject(DeviceExtension->ScanThread, Executive, KernelMode, FALSE, NULL);
//    };

} /* HGM_RemoveDevice */

/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_PnP |
 *
 *          Plug and Play dispatch routine for this driver.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DELETE_PENDING | PnP IRP received after device was removed
 *  @rvalue   ???   | Return from IoCallDriver() or HGM_InitDevice()
 *
 *****************************************************************************/
NTSTATUS  EXTERNAL
    HGM_PnP
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus;
    PDEVICE_EXTENSION DeviceExtension;
    KEVENT            StartEvent;

    PAGED_CODE();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_PnP(DeviceObject=0x%x,Irp=0x%x)",\
                    DeviceObject, Irp ));
    /*
     * Get a pointer to the device extension
     */
    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         * Someone sent us another plug and play IRP after removed
         */

        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_PnP: PnP IRP after device was removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else
    {
        PIO_STACK_LOCATION IrpStack;

        /*
         * Get a pointer to the current location in the Irp
         */
        IrpStack = IoGetCurrentIrpStackLocation (Irp);

        switch(IrpStack->MinorFunction)
        {
            case IRP_MN_START_DEVICE:

                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_START_DEVICE"));
                /*
                 * We cannot touch the device (send it any non pnp irps) until a
                 * start device has been passed down to the lower drivers.
                 */
                KeInitializeEvent(&StartEvent, NotificationEvent, FALSE);

                IoCopyCurrentIrpStackLocationToNext (Irp);
                IoSetCompletionRoutine (Irp, HGM_PnPComplete, &StartEvent, TRUE, TRUE, TRUE);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                if( NT_SUCCESS(ntStatus ) )
                {
                    ntStatus = KeWaitForSingleObject
                               (
                               &StartEvent,
                               Executive,   /* Waiting for reason of a driver */
                               KernelMode,  /* Waiting in kernel mode         */
                               FALSE,       /* No allert                      */
                               NULL         /* No timeout                     */
                               );
                }

                if(NT_SUCCESS(ntStatus))
                {
                    ntStatus = Irp->IoStatus.Status;
                }

                if(NT_SUCCESS (ntStatus))
                {
                    /*
                     * As we are now back from our start device we can do work.
                     */
                    DeviceExtension->ScanThread = NULL;
                    ntStatus = HGM_InitDevice (DeviceObject, Irp);
                    ntStatus = HGM_ActivateDevice(DeviceObject);
                } else
                {
                    HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                                   ("HGM_Pnp: IRP_MN_START_DEVICE ntStatus =0x%x",\
                                    ntStatus));
                }


                DeviceExtension->fStarted = TRUE;

                /*
                 *      Return Status
                 */
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = ntStatus;
                IoCompleteRequest (Irp, IO_NO_INCREMENT);

                break;

            case IRP_MN_STOP_DEVICE:

                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_STOP_DEVICE"));
                /*
                 * After the start IRP has been sent to the lower driver object, the bus may
                 * NOT send any more IRPS down ``touch'' until another START has occured.
                 * Whatever access is required must be done before Irp passed on.
                 */

                DeviceExtension->fStarted = FALSE;

                /*
                 * We don't need a completion routine so fire and forget.
                 * Set the current stack location to the next stack location and
                 * call the next device object.
                 */

                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;

            case IRP_MN_SURPRISE_REMOVAL:
                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_SURPRISE_REMOVAL"));

                HGM_RemoveDevice(DeviceExtension);

                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoSkipCurrentIrpStackLocation(Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);

                break;

            case IRP_MN_REMOVE_DEVICE:
                HGM_DBGPRINT(FILE_PNP | HGM_BABBLE,\
                               ("HGM_Pnp: IRP_MN_REMOVE_DEVICE"));

                /*
                 * The PlugPlay system has dictacted the removal of this device. We
                 * have no choice but to detach and delete the device object.
                 * (If we wanted to express an interest in preventing this removal,
                 * we should have filtered the query remove and query stop routines.)
                 * Note: we might receive a remove WITHOUT first receiving a stop.
                 */

                /*
                 *  Make sure we do not allow more IRPs to start touching the device
                 */
                DeviceExtension->fRemoved = TRUE;

                /*
                 * Stop the device without touching the hardware.
                 */
                HGM_RemoveDevice(DeviceExtension);

                /*
                 * Send on the remove IRP
                 */
                IoSkipCurrentIrpStackLocation (Irp);
                ntStatus = IoCallDriver (GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);


                /*
                 *  Remove this IRPs hold which should leave the initial 1 plus 
                 *  any other IRP holds.
                 */
                {
                    LONG RequestCount = InterlockedDecrement( &DeviceExtension->RequestCount );
                    ASSERT( RequestCount > 0 );
                }

                /*
                 *  If someone has already started, wait for them to finish
                 */
                if( InterlockedDecrement( &DeviceExtension->RequestCount ) > 0 )
                {
                    KeWaitForSingleObject( &DeviceExtension->RemoveEvent,
                        Executive, KernelMode, FALSE, NULL );
                }

                ntStatus = STATUS_SUCCESS;

                HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT_STATUSOK, "HGM_PnP Exit 1", ntStatus);

                return ntStatus;

            default:
                HGM_DBGPRINT(FILE_PNP | HGM_WARN,\
                               ("HGM_PnP: IrpStack->MinorFunction Not handled 0x%x", \
                                IrpStack->MinorFunction));

                IoSkipCurrentIrpStackLocation (Irp);

                ntStatus = IoCallDriver(GET_NEXT_DEVICE_OBJECT(DeviceObject), Irp);
                break;
        }

        HGM_DecRequestCount( DeviceExtension );
    }

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT, "HGM_PnP", ntStatus);

    return ntStatus;
} /* HGM_PnP */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_InitDevice |
 *
 *          Get the device information and attempt to initialize a configuration
 *          for a device.  If we cannot identify this as a valid HID device or
 *          configure the device, our start device function is failed.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   STATUS_DEVICE_CONFIGURATION_ERROR | Resources overlap
 *  @rvalue   ???            | Return from HGM_GetResources() or HGM_JoystickConfig()
 *
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_InitDevice
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    ULONG Id = 1;
    PDEVICE_EXTENSION   DeviceExtension;
    PDEVICE_OBJECT PDO;
    NTSTATUS status;
    UCHAR Type;
    UCHAR noFF=0;
    UCHAR noAx=0;
    UCHAR Psx2=0;
    UCHAR allanalog=0;
    UCHAR Delay=4;
    UCHAR ScanMode=0;
    UCHAR death=10;
    ULONG uId;
    ULONG num;
    ULONG Ejes = 0x03020100;
    ULONG puerto;
    WCHAR numberBuffer[10];
    /*
     * Get a pointer to the device extension
     */

    PDO = ((PHID_DEVICE_EXTENSION)(DeviceObject->DeviceExtension))->PhysicalDeviceObject;

    status = IoGetDeviceProperty(PDO, DevicePropertyUINumber, sizeof(ULONG),
                        (PVOID) &uId, &num);

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION(DeviceObject);

    Id = uId;
    DeviceExtension->SerialNo = Id;
    DeviceExtension->DeviceID = Id;

    ExAcquireFastMutex (&Global.Mutex);
    {
        OBJECT_ATTRIBUTES oa;
        OBJECT_ATTRIBUTES oa2;
        UNICODE_STRING sam;
        UNICODE_STRING numero;
        UNICODE_STRING valor0;
        UNICODE_STRING valor1;
        UNICODE_STRING valor2;
        UNICODE_STRING valor3;
        UNICODE_STRING valor4;
        UNICODE_STRING valor5;
        UNICODE_STRING valor6;
        UNICODE_STRING valor7;
        UNICODE_STRING valor8;
        UNICODE_STRING valor9;
        UNICODE_STRING valor10;
        UNICODE_STRING valor11;
        ULONG dataLength = 16;
        ULONG structLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + \
        dataLength + sizeof(UNICODE_NULL);
        HANDLE hKey;
        HANDLE hKey2;
        NTSTATUS status;
        PKEY_VALUE_PARTIAL_INFORMATION pPartialValue = NULL;
        pPartialValue = (PKEY_VALUE_PARTIAL_INFORMATION) \
                        ExAllocatePool(PagedPool, structLength);

        RtlInitUnicodeString(&sam, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\MediaProperties\\PrivateProperties\\Joystick\\NTPAD");

        numero.Buffer = numberBuffer;
        numero.MaximumLength = 20;
        RtlIntegerToUnicodeString(Id, 0, &numero);

        RtlInitUnicodeString(&valor0, L"Death");
        RtlInitUnicodeString(&valor1, L"Tipo");
        RtlInitUnicodeString(&valor2, L"Ejes");
        RtlInitUnicodeString(&valor3, L"Allanalog");
        RtlInitUnicodeString(&valor4, L"NoFF");
        RtlInitUnicodeString(&valor5, L"Puerto");
        RtlInitUnicodeString(&valor6, L"NoAx");
        RtlInitUnicodeString(&valor7, L"Id");
        RtlInitUnicodeString(&valor8, L"Map");
        RtlInitUnicodeString(&valor9, L"Psx2");
        RtlInitUnicodeString(&valor10, L"Delay");
        RtlInitUnicodeString(&valor11, L"ScanMode");

        InitializeObjectAttributes(&oa, &sam, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

        status = ZwOpenKey(&hKey, KEY_READ, &oa);

        if ( NT_SUCCESS(status) )
        {
             InitializeObjectAttributes(&oa2, &numero, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, hKey, NULL);

             status = ZwOpenKey(&hKey2, KEY_READ, &oa2);
        
             if ( NT_SUCCESS(status) )
             {
                ZwQueryValueKey(hKey2, &valor0, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                death = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor1, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                Type = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor2, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                Ejes = *((PULONG)pPartialValue->Data);

                ZwQueryValueKey(hKey2, &valor3, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                allanalog = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor4, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                noFF = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor5, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                RtlCopyMemory(&puerto, pPartialValue->Data, 4);
        
                ZwQueryValueKey(hKey2, &valor6, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                noAx = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor7, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                DeviceExtension->SerialNo = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor8, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                RtlCopyMemory(DeviceExtension->psx.buttonmap, pPartialValue->Data, dataLength);

                ZwQueryValueKey(hKey2, &valor9, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                Psx2 = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor10, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                Delay = pPartialValue->Data[0];

                ZwQueryValueKey(hKey2, &valor11, \
                                KeyValuePartialInformation, pPartialValue, \
                                                structLength, &dataLength);
                ScanMode = pPartialValue->Data[0];

                ZwClose(hKey2);
                ZwClose(hKey);
             } else {
               ZwClose(hKey);
               goto Siguiente;
             };
             
        } else {
Siguiente:
                Type = 0;
                puerto = 0x378;
                Delay = 4;
                ScanMode = 0;
                death = 10;
        };
        ExFreePool(pPartialValue);
    };
    DeviceExtension->psx.errors = 0;
    DeviceExtension->psx.Initialized = 0;
    DeviceExtension->psx.laststatus = STATUS_SUCCESS;
    
    switch(puerto)
    {
      case 0:
       DeviceExtension->psx.base = (PUCHAR) 0x378;
       break;
      case 1:
       DeviceExtension->psx.base = (PUCHAR) 0x278;
       break;
      case 2:
       DeviceExtension->psx.base = (PUCHAR) 0x3BC;
       break;
      case 3:
       DeviceExtension->psx.base = (PUCHAR) 0x2BC;
       break;
      case 4:
       DeviceExtension->psx.base = (PUCHAR) 0x140;
       break;
      default:
       DeviceExtension->psx.base = (PUCHAR) puerto;                 
    };
                                    
    DeviceExtension->psx.type = Type;
    DeviceExtension->psx.noFF = noFF;
    DeviceExtension->psx.noAx = noAx;
    DeviceExtension->psx.Psx2 = Psx2;
    DeviceExtension->psx.Delay = Delay;
    DeviceExtension->psx.ScanMode = ScanMode;
    DeviceExtension->psx.allanalog = allanalog;
    if((Ejes == 0) || (Ejes == 1)) // version anterior
      Ejes = 0x03020100;
    DeviceExtension->psx.Ejes = Ejes;

    DeviceExtension->psx.min = 127 - (death * 255 / 100);
    DeviceExtension->psx.max = 127 + (death * 255 / 100);


    ExReleaseFastMutex (&Global.Mutex);
    return( STATUS_SUCCESS );

} /* HGM_InitDevice */


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_PnPComplete |
 *
 *          Completion routine for PnP IRPs.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *  @rvalue STATUS_MORE_PROCESSING_REQUIRED | We want the IRP back
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_PnPComplete
    (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             Irp,
    IN PVOID            Context
    )
{
    NTSTATUS ntStatus = STATUS_MORE_PROCESSING_REQUIRED;

    PAGED_CODE();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("HGM_PnPComplete(DeviceObject=0x%x,Irp=0x%x,Context=0x%x)", \
                    DeviceObject, Irp, Context));

    UNREFERENCED_PARAMETER (DeviceObject);
    KeSetEvent ((PKEVENT) Context, 0, FALSE);

    /*
     *  If the lower driver returned PENDING, mark our stack location as
     *  pending also. This prevents the IRP's thread from being freed if
     *  the client's call returns pending.
     */
    if(Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }

    HGM_EXITPROC(FILE_IOCTL|HGM_FEXIT, "HGM_PnpComplete", ntStatus);

    return ntStatus;
}


/*****************************************************************************
 *
 *  @doc    EXTERNAL
 *
 *  @func   NTSTATUS  | HGM_Power |
 *
 *          The power dispatch routine.
 *          <nl>This driver does not recognize power IRPS.  It merely sends them down,
 *          unmodified to the next device on the attachment stack.
 *          As this is a POWER irp, and therefore a special irp, special power irp
 *          handling is required. No completion routine is required.
 *
 *  @parm   IN PDEVICE_OBJECT | DeviceObject |
 *
 *          Pointer to the device object.
 *
 *  @parm   IN PIRP | Irp |
 *
 *          Pointer to an I/O request packet.
 *
 *
 *  @rvalue   STATUS_SUCCESS | success
 *  @rvalue   ???            | Return from PoCallDriver()
 *
 *****************************************************************************/
NTSTATUS INTERNAL
    HGM_Power
    (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
{
    PDEVICE_EXTENSION  DeviceExtension;
    NTSTATUS           ntStatus;

    PAGED_CODE ();

    HGM_DBGPRINT(FILE_PNP | HGM_FENTRY,\
                   ("Enter HGM_Power(DeviceObject=0x%x,Irp=0x%x)",DeviceObject, Irp));

    DeviceExtension = GET_MINIDRIVER_DEVICE_EXTENSION (DeviceObject);

    /*
     * Since we do not know what to do with the IRP, we should pass
     * it on along down the stack.
     */

    ntStatus = HGM_IncRequestCount( DeviceExtension );
    if (!NT_SUCCESS (ntStatus))
    {
        /*
         * Someone sent us another plug and play IRP after removed
         */

        HGM_DBGPRINT(FILE_PNP | HGM_ERROR,\
                       ("HGM_Power: PnP IRP after device was removed\n"));
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
    } else
    {
        IoSkipCurrentIrpStackLocation (Irp);

        /*
         * Power IRPS come synchronously; drivers must call
         * PoStartNextPowerIrp, when they are ready for the next power irp.
         * This can be called here, or in the completetion routine.
         */
        PoStartNextPowerIrp (Irp);

        /*
         * NOTE!!! PoCallDriver NOT IoCallDriver.
         */
        ntStatus =  PoCallDriver (GET_NEXT_DEVICE_OBJECT (DeviceObject), Irp);

        HGM_DecRequestCount( DeviceExtension );
    }


    HGM_EXITPROC(FILE_IOCTL | HGM_FEXIT, "HGM_Power", ntStatus);
    return ntStatus;
} /* HGM_Power */


