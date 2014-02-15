#ifndef DESCRIPTORS_H
#define DESCRIPTORS_H

	/* Includes: */
		#include <avr/pgmspace.h>
		#include <LUFA/Drivers/USB/USB.h>

	/* Type Defines: */
		/** Type define for the device configuration descriptor structure. This must be defined in the
		 *  application code, as the configuration descriptor contains several sub-descriptors which
		 *  vary between devices, and which describe the device's usage to the host.
		 */
		typedef struct
		{
			USB_Descriptor_Configuration_Header_t Config;

			// UUAdapter Interface
			USB_Descriptor_Interface_t            UUAdapterInterface;
		} USB_Descriptor_Configuration_t;

		/** Enum for the device string descriptor IDs within the device. Each string descriptor should
		 *  have a unique ID index associated with it, which can be used to refer to the string from
		 *  other descriptors.
		 */
		enum StringDescriptors_t
		{
		    STRING_ID_Language      = 0, /**< Supported Languages string descriptor ID (must be zero) */
		    STRING_ID_Manufacturer  = 1, /**< Manufacturer string ID */
		    STRING_ID_Product       = 2, /**< Product string ID */
		    STRING_ID_Serial        = 3, /**< Serial number string ID */
		};

	/* Function Prototypes: */
		uint16_t CALLBACK_USB_GetDescriptor(const uint16_t wValue,
		                                    const uint8_t wIndex,
		                                    const void** const DescriptorAddress)
		                                    ATTR_WARN_UNUSED_RESULT ATTR_NON_NULL_PTR_ARG(3);
#endif // DESCRIPTORS_H

