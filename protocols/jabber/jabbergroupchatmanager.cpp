/*
    jabbergroupchatmanager.cpp - Jabber Message Manager for group chats

    Copyright (c) 2004 by Till Gerken            <till@tantalo.net>

    Kopete    (c) 2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "jabbergroupchatmanager.h"

#include <qptrlist.h>
#include "kopetemessagemanagerfactory.h"
#include "kopeteview.h"
#include "jabberprotocol.h"
#include "jabberaccount.h"
#include "jabbercontact.h"

JabberGroupChatManager::JabberGroupChatManager ( JabberProtocol *protocol, const JabberBaseContact *user,
											 KopeteContactPtrList others, XMPP::Jid roomJid, const char *name )
											 : KopeteMessageManager ( user, others, protocol, 0, name )
{
	kdDebug ( JABBER_DEBUG_GLOBAL ) << k_funcinfo << "New message manager for " << user->contactId () << endl;

	mRoomJid = roomJid;

	// make sure Kopete knows about this instance
	KopeteMessageManagerFactory::factory()->addKopeteMessageManager ( this );

	connect ( this, SIGNAL ( messageSent ( KopeteMessage &, KopeteMessageManager * ) ),
			  this, SLOT ( slotMessageSent ( KopeteMessage &, KopeteMessageManager * ) ) );

	updateDisplayName ();

}

void JabberGroupChatManager::updateDisplayName ()
{
	kdDebug ( JABBER_DEBUG_GLOBAL ) << k_funcinfo << endl;

	setDisplayName ( mRoomJid.full () );

}

const JabberBaseContact *JabberGroupChatManager::user () const
{

	return static_cast<const JabberBaseContact *>(KopeteMessageManager::user());

}

JabberAccount *JabberGroupChatManager::account () const
{

	return static_cast<JabberAccount *>(KopeteMessageManager::account ());

}

void JabberGroupChatManager::slotMessageSent ( KopeteMessage &message, KopeteMessageManager * )
{

	if( account()->isConnected () )
	{
		XMPP::Message jabberMessage;

		XMPP::Jid jid ( message.from()->contactId () );
		jabberMessage.setFrom ( jid );

		XMPP::Jid toJid ( mRoomJid );

		jabberMessage.setTo ( toJid );

		jabberMessage.setSubject ( message.subject () );
		jabberMessage.setTimeStamp ( message.timestamp () );

		if ( message.plainBody().find ( "-----BEGIN PGP MESSAGE-----" ) != -1 )
		{
			/*
			 * This message is encrypted, so we need to set
			 * a fake body indicating that this is an encrypted
			 * message (for clients not implementing this
			 * functionality) and then generate the encrypted
			 * payload out of the old message body.
			 */

			// please don't translate the following string
			jabberMessage.setBody ( "This message is encrypted.", false );

			QString encryptedBody = message.plainBody ();

			// remove PGP header and footer from message
			encryptedBody.truncate ( encryptedBody.length () - QString("-----END PGP MESSAGE-----").length () - 2 );
			encryptedBody = encryptedBody.right ( encryptedBody.length () - encryptedBody.find ( "\n\n" ) - 2 );

			// assign payload to message
			jabberMessage.setXEncrypted ( encryptedBody );
        }
        else
        {
			// this message is not encrypted
			jabberMessage.setBody ( message.plainBody (), false );
        }

		jabberMessage.setType ( "groupchat" );

		// send the message
		account()->client()->sendMessage ( jabberMessage );

		// tell the manager that we sent successfully
		messageSucceeded ();
	}
	else
	{
		account()->errorConnectFirst ();

		// FIXME: there is no messageFailed() yet,
		// but we need to stop the animation etc.
		messageSucceeded ();
	}

}

#include "jabbergroupchatmanager.moc"

// vim: set noet ts=4 sts=4 sw=4:

