/*
    gwcontact.h - Kopete GroupWise Protocol

    Copyright (c) 2004      SUSE Linux AG	 	 http://www.suse.com
    
    Based on Testbed   
    Copyright (c) 2003      by Will Stephenson		 <will@stevello.free-online.co.uk>
    
    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This library is free software; you can redistribute it and/or         *
    * modify it under the terms of the GNU General Public                   *
    * License as published by the Free Software Foundation; either          *
    * version 2 of the License, or (at your option) any later version.      *
    *                                                                       *
    *************************************************************************
*/

#ifndef GW_CONTACT_H
#define GW_CONTACT_H

#include <qdict.h>
#include <qmap.h>

#include "kopetecontact.h"
#include "kopetemessage.h"

#include "gwfield.h"

class KAction;
class KActionCollection;
class KopeteAccount;
class GroupWiseMessageManager;
class KopeteMetaContact;

/**
@author Will Stephenson
*/
class GroupWiseContact : public KopeteContact
{
	Q_OBJECT
public:
	/** 
	 * Constructor
	 * @param account The GroupWiseAccount this belongs to.
	 * @param uniqueName The unique identifier for this contact, in GroupWise terms, the DN.
	 * @param parent The KopeteMetaContact this contact is part of.
	 * @param displayName The display name given to this contact by the protocol.
	 * @param objectId The contact's numeric object ID.
	 * @param parentId The ID of this contact's parent (folder).
	 * @param sequence This contact's sequence number (The position it appears in within its parent).
	 */
	GroupWiseContact( KopeteAccount* account, const QString &uniqueName, 
			KopeteMetaContact *parent, 
			const QString &displayName, const int objectId, const int parentId, const int sequence );

    ~GroupWiseContact();

	/** 
	 * Access this contact's KopeteProtocol subclass
	 */
	GroupWiseProtocol * protocol();

	/**
	 * Update the contact's status and metadata from the supplied fields
	 */
	void updateDetails( const ContactDetails & details );
	
	virtual bool isReachable();
	/**
	 * Serialize the contact's data into a key-value map
	 * suitable for writing to a file
	 */
    virtual void serialize(QMap< QString, QString >& serializedData,
			QMap< QString, QString >& addressBookData);
	/**
	 * Return the actions for this contact
	 */
	virtual QPtrList<KAction> *customContextMenuActions();
	
	/**
	 * Returns a KopeteMessageManager associated with this contact
	 */
	virtual KopeteMessageManager *manager( bool canCreate = false );
	/** 
	 * Locate or create a messagemanager for the specified group of contacts
	 */
	GroupWiseMessageManager *manager ( KopeteContactPtrList chatMembers, bool canCreate = false );
	
	
	/**
	 * Received a message from the server.
	 * Find the conversation that this message belongs to, and display it there.
	 */
	void handleIncomingMessage( const ConferenceEvent & event );

public slots:
	/**
	 * Transmits an outgoing message to the server 
	 * Called when the chat window send button has been pressed
	 * (in response to the relevant KopeteMessageManager signal)
	 */
	void sendMessage( KopeteMessage &message );
protected:
	/**
	 * Returns the KopeteMessageManager for the GroupWise conference with the supplied GUID, or creates a new one.
	 */
	GroupWiseMessageManager *manager( const QString & guid, bool canCreate = false );
	// debug function to see what message managers we have on the server
	void dumpManagers();
protected slots:
	/**
	 * Show the settings dialog
	 */
	void showContactSettings();
	/**
	 * A message manager was instantiated as a conference on the server, so record it.
	 */
	void slotConferenceCreated();
	/**
	 * Notify the contact that a KopeteMessageManager was
	 * destroyed - probably by the chatwindow being closed
	 */
	void slotMessageManagerDeleted( QObject *sender );
	
protected:
	KActionCollection* m_actionCollection;
	
	int m_objectId;
	int m_parentId;
	int m_sequence;
	
	KAction* m_actionPrefs;
	QDict< GroupWiseMessageManager > m_msgManagers;
	// message managers that don't exist on the server yet
	//QPtrList m_pendingManagers;
};

#endif
