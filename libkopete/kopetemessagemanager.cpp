/*
    kopetemessagemanager.cpp - Manages all chats

    Copyright   : (c) 2002 by Martijn Klingens <klingens@kde.org>
                  (c) 2002 by Duncan Mac-Vicar Prett <duncan@kde.org>
                  (c) 2002 by Daniel Stone <dstone@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <kdebug.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <qregexp.h>
#include <qmap.h>

#include "kopeteaway.h"
#include "kopetemessagelog.h"
#include "kopetemessagemanager.h"
#include "kopetemessagemanagerfactory.h"
#include "kopeteprefs.h"
#include "kopeteprotocol.h"
#include "kopetemetacontact.h"

struct KMMPrivate
{
	KopeteContactPtrList mContactList;
	const KopeteContact *mUser;
	KopeteMessageLog *mLogger;
	QMap<const KopeteContact *, QStringList> resources;
	KopeteProtocol *mProtocol;
	int mId;
	bool mLog;
	bool isEmpty;
	bool mCanBeDeleted;
};

KopeteMessageManager::KopeteMessageManager( const KopeteContact *user,
	KopeteContactPtrList others, KopeteProtocol *protocol, int id,
	QObject *parent, const char *name )
: QObject( parent, name )
{
	d = new KMMPrivate;
	d->mContactList = others;
	d->mUser = user;
	d->mProtocol = protocol;
	d->mId = id;
	d->mLog = true;
	d->isEmpty= others.isEmpty();
	d->mCanBeDeleted = false;

	connect( this, SIGNAL(readMessages( KopeteMessageManager*, bool )), KopeteViewManager::viewManager(), SLOT(readMessages(KopeteMessageManager*,bool)));
	connect( this, SIGNAL(messageAppended( KopeteMessage &, KopeteMessageManager *) ), KopeteViewManager::viewManager(), SLOT( messageAppended( KopeteMessage &, KopeteMessageManager *) ) );

	// Replace '.', '/' and '~' in the user id with '-' to avoid possible
	// directory traversal, although appending '.log' and the rest of the
	// code should really make overwriting files possible anyway.
	KopeteContact *c = others.first();
	QString logFileName = QString::fromLatin1( "kopete/" ) + QString::fromLatin1( c->protocol()->pluginId() ) +
		QString::fromLatin1( "/" ) + c->contactId().replace( QRegExp( QString::fromLatin1( "[./~]" ) ),
		QString::fromLatin1( "-" ) ) + QString::fromLatin1( ".log" );
	d->mLogger = new KopeteMessageLog( logFileName, this );

//	connect(protocol, SIGNAL(destroyed()), this, SLOT(slotProtocolUnloading()));

	kdDebug(14010) << k_funcinfo << endl;
}

KopeteMessageManager::~KopeteMessageManager()
{
	kdDebug(14010) << k_funcinfo << endl;
	if (!d) return;
	d->mCanBeDeleted = false; //prevent double deletion
	KopeteMessageManagerFactory::factory()->removeSession( this );
	emit(closing( this ) );
	delete d;
}

void KopeteMessageManager::setLogging( bool on )
{
	d->mLog = on;
}

bool KopeteMessageManager::logging() const
{
	return d->mLog;
}

const QString KopeteMessageManager::chatName()
{
	QString chatName, nextDisplayName;

	KopeteContact *c = d->mContactList.first();
	if( c->metaContact() )
		chatName = c->metaContact()->displayName();
	else
		chatName = c->displayName();

	//If we have only 1 contact, add the status of him
	if( d->mContactList.count() == 1 )
	{
		chatName.append( QString::fromLatin1(" (") + c->statusText() + QString::fromLatin1(")") );
	}
	else
	{
		while( ( c = d->mContactList.next() ) )
		{
			if( c->metaContact() )
				nextDisplayName = c->metaContact()->displayName();
			else
				nextDisplayName = c->displayName();
			chatName.append( QString::fromLatin1( ", " ) ).append( nextDisplayName );
		}
	}

	return chatName;
}

const KopeteContactPtrList& KopeteMessageManager::members() const
{
	return d->mContactList;
}

const KopeteContact* KopeteMessageManager::user() const
{
	return d->mUser;
}

KopeteProtocol* KopeteMessageManager::protocol() const
{
	return d->mProtocol;
}

int KopeteMessageManager::mmId() const
{
	return d->mId;
}

void KopeteMessageManager::setMMId( int id )
{
	d->mId = id;
}

void KopeteMessageManager::sendMessage(KopeteMessage &message)
{
	KopeteMessage sentMessage = message;
	emit messageSent(sentMessage, this);

	if ( KopetePrefs::prefs()->soundNotify() )
	{
		if ( !protocol()->isAway() || KopetePrefs::prefs()->soundIfAway() )
			KNotifyClient::event( QString::fromLatin1( "kopete_outgoing" ) );
	}
}

void KopeteMessageManager::appendMessage( KopeteMessage &msg )
{
	kdDebug(14010) << k_funcinfo << endl;

	emit messageAppended( msg, this );
	
	if( msg.direction() == KopeteMessage::Inbound )
		emit( messageReceived( msg, this ) );

	if( d->mLogger && d->mLog )
		d->mLogger->append( msg );

}

void KopeteMessageManager::addContact( const KopeteContact *c )
{
	if ( d->mContactList.contains(c) )
	{
		kdDebug(14010) << k_funcinfo << "Contact already exists" <<endl;
		emit contactAdded(c);
	}
	else
	{
		if(d->mContactList.count()==1 && d->isEmpty)
		{
			KopeteContact *old=d->mContactList.first();
			kdDebug(14010) << k_funcinfo << old->displayName() << " left and " << c->displayName() << " joined " <<endl;
			d->mContactList.remove(old);
			d->mContactList.append(c);
			disconnect (old->metaContact(), SIGNAL(displayNameChanged(KopeteMetaContact *, const QString)), this, SIGNAL(chatNameChanged()));
			connect (c->metaContact(), SIGNAL(displayNameChanged(KopeteMetaContact *, const QString)), this, SIGNAL(chatNameChanged()));
			emit contactAdded(c);
			emit contactRemoved(old);
		}
		else
		{
			kdDebug(14010) << k_funcinfo << "Contact Joined session : " <<c->displayName() <<endl;
			connect (c->metaContact(), SIGNAL(displayNameChanged(KopeteMetaContact *, const QString)), this, SIGNAL(chatNameChanged()));
			d->mContactList.append(c);
			emit contactAdded(c);
		}
	}
	d->isEmpty=false;
}

void KopeteMessageManager::removeContact( const KopeteContact *c )
{
	if(!c || !d->mContactList.contains(c))
		return;

	if(d->mContactList.count()==1)
	{
		kdDebug(14010) << k_funcinfo << "Contact not removed. Keep always one contact" <<endl;
		d->isEmpty=true;
	}
	else
	{
		d->mContactList.remove( c );
		disconnect (c->metaContact(), SIGNAL(displayNameChanged(KopeteMetaContact *, const QString)), this, SIGNAL(chatNameChanged()));
	}
	emit contactRemoved(c);
}

void KopeteMessageManager::receivedTypingMsg( const KopeteContact *c , bool t )
{
	emit(remoteTyping( c, t ));
}

void KopeteMessageManager::receivedTypingMsg( const QString &contactId , bool t )
{
	for( KopeteContact *it = d->mContactList.first(); it; it = d->mContactList.next() )
	{
		if( it->contactId() == contactId )
		{
			receivedTypingMsg( it, t );
			return;
		}
	}
}

void KopeteMessageManager::typing ( bool t )
{
	emit typingMsg(t);
}

void KopeteMessageManager::setCanBeDeleted ( bool b )
{
	d->mCanBeDeleted = b;
	if(b)
		deleteLater();
}

void KopeteMessageManager::slotReadMessages()
{
	emit( readMessages( this, true ) );
}

KopeteMessage KopeteMessageManager::currentMessage()
{
	//return KopeteViewManager::viewManager()->view(this)->currentMessage();
}

void KopeteMessageManager::setCurrentMessage(const KopeteMessage &message)
{
	//KopeteViewManager::viewManager()->view(this)->setCurrentMessage(message);
}

#include "kopetemessagemanager.moc"

// vim: set noet ts=4 sts=4 sw=4:
