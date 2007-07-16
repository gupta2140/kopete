/*
    cryptographyguiclient.h

    Copyright (c) 2004      by Olivier Goffart        <ogoffart@kde.org>

    Kopete    (c) 2003-2004 by the Kopete developers  <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/
#ifndef CRYPTOGUICLIENT_H
#define CRYPTOGUICLIENT_H

#include <qobject.h>
#include <kxmlguiclient.h>
#include <ktoggleaction.h>

namespace Kopete { class ChatSession; }


/**
 *@author Olivier Goffart
 */
class CryptographyGUIClient : public QObject, public KXMLGUIClient
{
Q_OBJECT
public:
	CryptographyGUIClient(Kopete::ChatSession *parent = 0);
	~CryptographyGUIClient();
	
	bool signing() { return m_signAction->isChecked(); }
	bool encrypting() { return m_encAction->isChecked(); }

private:
	KToggleAction *m_encAction;
	KToggleAction *m_signAction;

private slots:
	void slotEncryptToggled();
	void slotSignToggled();
};

#endif
