/***************************************************************************
                          wpcontact.h  -  description
                             -------------------
    begin                : Fri Apr 12 2002
    copyright            : (C) 2002 by Gav Wood
    email                : gav@indigoarchive.net
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __WPCONTACT_H
#define __WPCONTACT_H

// KDE Includes
#include <kaction.h>

// Qt Includes
#include <qvaluestack.h>
#include <qdatetime.h>
#include <qptrlist.h>
#include <qtimer.h>
#include <qstringlist.h>

// Kopete Includes
#include "kopetecontact.h"
#include "kopetecontactlist.h"
#include "kopetemessagemanagerfactory.h"
#include "kopetemessagemanager.h"
#include "kopetemessage.h"

// Local Includes
#include "wpprotocol.h"
#include "wpuserinfo.h"

class QTimer;
class QListView;
class QListViewItem;
class KPopupMenu;
class KAction;
class KopeteMetaContact;

class WPContact: public KopeteContact
{
	Q_OBJECT

public:
	WPContact(KopeteAccount *account, const QString &userId, const QString &fullName, KopeteMetaContact *metaContact, KopeteContact::AddMode mode );

//	virtual bool isOnline() const;
	virtual bool isReachable();
	virtual KActionCollection *customContextMenuActions();
	virtual KopeteMessageManager *manager(bool canCreate = false);
	virtual void serialize(QMap<QString, QString> &serializedData, QMap<QString, QString> &addressBookData);

public slots:
	virtual void slotUserInfo();
	void slotCheckStatus();	// the call back for the checkStatus timer
	void slotNewMessage(const QString &Body, const QDateTime &Arrival);

signals:
	void messageSuccess();

private slots:
	void slotMessageManagerDestroyed();
	void slotSendMessage(KopeteMessage &message);
	void slotCloseUserInfoDialog(); // Called when the userinfo dialog is getting closed

private:
	bool myWasConnected;	// true if protocol connected at last check

	QTimer checkStatus;		// checks the status of this contact every second or so
	KActionCollection *myActionCollection;
							// holds all the protocol specific actions (not many!)
	KopeteMessageManager *m_manager;
							// holds the two message managers - one for email and one for chat
	WPUserInfo *m_infoDialog;
};

#endif
