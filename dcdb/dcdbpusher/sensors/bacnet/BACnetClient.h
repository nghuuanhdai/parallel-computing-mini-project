//================================================================================
// Name        : BACnetClient.h
// Author      : Micha Mueller
// Contact     : info@dcdb.it
// Copyright   : Leibniz Supercomputing Centre
// Description : Header file for BACnetClient class.
//================================================================================

//================================================================================
// This file is part of DCDB (DataCenter DataBase)
// Copyright (C) 2018-2019 Leibniz Supercomputing Centre
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//================================================================================

#ifndef BACNETCLIENT_H_
#define BACNETCLIENT_H_

#include "../../includes/EntityInterface.h"

#include "bacnet/apdu.h"
#include "bacnet/bacenum.h"
#include "bacnet/datalink.h"

/**
 * @brief Client to handle BACnet protocol communication. Only one instance
 *        allowed!
 *
 * @details Had to make some member variables static because of BACnet Stack
 *          handler function requirements. This should be no problem as we are
 *          using only one instance of BACnetClient with serialized access
 *          (_strand). One should ensure to keep the single instance property in
 *          the future.
 *
 * @ingroup bacnet
 */
class BACnetClient : public EntityInterface {

      public:
	BACnetClient(const std::string &name = "BACnetClient");
	BACnetClient(const BACnetClient &other) = delete;
	virtual ~BACnetClient();
	BACnetClient &operator=(const BACnetClient &other) = delete;

	/**
	 * Override C++'s name hiding
	 */
	using EntityInterface::init;

	/**
	 * @brief Initialize datalink layer and address cache.
	 *
	 * @details We assume BACnet/IP protocol is used. Also you need to compile
	 *          with environment variable BACNET_ADDRESS_CACHE_FILE set to
	 *          enable initialization of address cache from file "address_cache".
	 *          Overloads EntityInterface::init().
	 *
	 * @param interface		Name of network interface to use.
	 * @param address_cache	(Path and) filename of the address cache file where
	 *                      the addresses of BACnet devices are stored.
	 * @param port			Which port to use of the interface.
	 * @param timeout		Number of milliseconds to wait for a packet when
	 *                      receiving.
	 * @param apdu_timeout	Number of milliseconds before timeout when sending.
	 * @param retries		Number of retries after an apdu timeout occurs.
	 */
	void init(std::string        interface,
		  const std::string &address_cache,
		  unsigned           port = 47808,
		  unsigned           timeout = 1000,
		  unsigned           apdu_timeout = 200,
		  unsigned           retries = 0);

	/**
	 * @brief Sends a READ_PROPERTY request for PROP_PRESENT_VALUE to specified
	 *        device and decodes the response (READ_PROPERTY_ACK).
	 *
	 * @param	deviceObjInstance Number of device from which to request
	 *                            the current value.
	 * @param	objInstance       Object instance
	 * @param	objType           Object type
	 * @param	objProperty       Object property
	 * @param	objIndex          Object index
	 *
	 * @return	The value sent as response from the device. If an error occurs a
	 *          runtime exception is thrown.
	 *
	 * @throws	Runtime error
	 */
	double readProperty(uint32_t           deviceObjInstance,
			    uint32_t           objInstance = 0,
			    BACNET_OBJECT_TYPE objType = OBJECT_DEVICE,
			    BACNET_PROPERTY_ID objProperty = PROP_PRESENT_VALUE,
			    int32_t            objIndex = BACNET_ARRAY_ALL);

	/**
     * @brief Print configuration of this class.
     *
     * @param ll Log severity level to be used from logger.
     */
	void printEntityConfig(LOG_LEVEL ll, unsigned int leadingSpaces) override;

      private:
	/* Handler to process incoming BACnet data */

	/**
	 * @brief Handler for a ReadProperty ACK.
	 * @details Here the actual processing of a valid response takes place.
	 *
	 * @param service_request	The contents of the service request.
	 * @param service_len		The length of the service_request.
	 * @param src				BACNET_ADDRESS of the source of the message.
	 * @param service_data		The BACNET_CONFIRMED_SERVICE_DATA information
	 *                          decoded from the APDU header of this message.
	 */
	static void readPropertyAckHandler(uint8_t *                          service_request,
					   uint16_t                           service_len,
					   BACNET_ADDRESS *                   src,
					   BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data);

	static void unrecognizedServiceHandler(uint8_t *                      service_request,
					       uint16_t                       service_len,
					       BACNET_ADDRESS *               src,
					       BACNET_CONFIRMED_SERVICE_DATA *service_data);
	static void errorHandler(BACNET_ADDRESS *   src,
				 uint8_t            invokeId,
				 BACNET_ERROR_CLASS error_class,
				 BACNET_ERROR_CODE  error_code);
	static void abortHandler(BACNET_ADDRESS *src,
				 uint8_t         invokeId,
				 uint8_t         abort_reason,
				 bool            server);
	static void rejectHandler(BACNET_ADDRESS *src,
				  uint8_t         invokeId,
				  uint8_t         reject_reason);

	uint8_t  _invokeId;
	unsigned _timeout;
	// static because of handler functions
	static double  _presentValue;
	static uint8_t _handlerTransmitBuffer[MAX_PDU];
	BACNET_ADDRESS _targetAddress; //store as member variable to enable access in handler methods
};

#endif /* BACNETCLIENT_H_ */
