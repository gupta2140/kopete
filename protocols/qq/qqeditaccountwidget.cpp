/*
    qqeditaccountwidget.cpp - QQ Account Widget

    Copyright (c) 2003      by Olivier Goffart       <ogoffart @ kde.org>
    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>

    Kopete    (c) 2002-2003 by the Kopete developers <kopete-devel@kde.org>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include "qqeditaccountwidget.h"

#include <qcheckbox.h>
#include <q3groupbox.h>
#include <qimage.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <q3listbox.h>
#include <qpushbutton.h>
#include <qregexp.h>
#include <qspinbox.h>
//Added by qt3to4:
#include <QPixmap>
#include <QVBoxLayout>
#include <QLatin1String>

#include <kfiledialog.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>
#include <kio/netaccess.h>
#include <kdebug.h>
#include <ktoolinvocation.h>
#include <kconfig.h>
#include <kpixmapregionselectordialog.h>

#include "kopeteuiglobal.h"
#include "kopeteglobal.h"

#include "kopetepasswordwidget.h"

#include "qqaccount.h"
#include "qqcontact.h"
#include "ui_qqeditaccountui.h"
#include "qqnotifysocket.h"
#include "qqprotocol.h"

// TODO: This was using KAutoConfig before, use KConfigXT instead.
class QQEditAccountWidgetPrivate
{
public:
	QQProtocol *protocol;
	Ui::QQEditAccountUI *ui;

	QString pictureUrl;
	QImage pictureData;
};

QQEditAccountWidget::QQEditAccountWidget( QQProtocol *proto, Kopete::Account *account, QWidget *parent )
: QWidget( parent ), KopeteEditAccountWidget( account )
{
	d = new QQEditAccountWidgetPrivate;

	d->protocol=proto;

	d->ui = new Ui::QQEditAccountUI();
	d->ui->setupUi( this );

	// FIXME: actually, I don't know how to set fonts for qlistboxitem - Olivier
	d->ui->label_font->hide();

	// default fields
	if ( account )
	{
		KConfigGroup * config=account->configGroup();
	
		d->ui->m_login->setText( account->accountId() );
		d->ui->m_password->load( &static_cast<QQAccount *>(account)->password() );


		//remove me after we can change account ids (Matt)
		d->ui->m_login->setDisabled( true );
		d->ui->m_autologin->setChecked( account->excludeConnect()  );
		if ( ( static_cast<QQAccount*>(account)->serverName() != "tcpconn.tencent.com" ) || ( static_cast<QQAccount*>(account)->serverPort() != 80) ) {
			d->ui->optionOverrideServer->setChecked( true );
		}

		QQContact *myself = static_cast<QQContact *>( account->myself() );

		d->ui->m_nickName->setText( myself->property( Kopete::Global::Properties::self()->nickName()).value().toString() );
		d->ui->m_phm->setText( config->readEntry("PHM") );
		d->ui->m_phh->setText( config->readEntry("PHH") );

		// FIXME: just try to load the property
		d->ui->m_country->setText( myself->property( d->protocol->propCountry ).value().toString() ); 
		d->ui->m_email->setText( myself->property( d->protocol->propEmail ).value().toString() ); 

		bool connected = account->isConnected();
		if ( connected )
		{
			d->ui->m_warning_1->hide();
			d->ui->m_warning_2->hide();
		}
		d->ui->m_nickName->setEnabled( connected );
		d->ui->m_allowButton->setEnabled( connected );
		d->ui->m_blockButton->setEnabled( connected );

		QQAccount *m_account = static_cast<QQAccount*>( account );
		d->ui->m_serverName->setText( m_account->serverName() );
		d->ui->m_serverPort->setValue( m_account->serverPort() );

		QStringList blockList = config->readEntry( "blockList", QStringList() );
		QStringList allowList = config->readEntry( "allowList", QStringList() );

		for ( QStringList::Iterator it = blockList.begin(); it != blockList.end(); ++it )
			d->ui->m_BL->insertItem( *it );

		for ( QStringList::Iterator it = allowList.begin(); it != allowList.end(); ++it )
			d->ui->m_AL->insertItem( *it );

		d->ui->m_blp->setChecked( config->readEntry( "BLP" ) == "BL" );

		d->pictureUrl = KStandardDirs::locateLocal( "appdata", "qqpicture-" +
			account->accountId().toLower().replace( QRegExp("[./~]" ), "-" ) + ".png" );
		d->ui->m_displayPicture->setPixmap( d->pictureUrl );

		d->ui->m_useDisplayPicture->setChecked( config->readEntry( "exportCustomPicture", false ));

		// Global Identity
		d->ui->m_globalIdentity->setChecked( config->readEntry("ExcludeGlobalIdentity", false) );
	}
	else
	{
		d->ui->tab_contacts->setDisabled( true );
		d->ui->m_nickName->setDisabled( true );
	}

	connect( d->ui->buttonRegister, SIGNAL(clicked()), this, SLOT(slotOpenRegister()));
	QWidget::setTabOrder( d->ui->m_login, d->ui->m_password->mRemembered );
	QWidget::setTabOrder( d->ui->m_password->mRemembered, d->ui->m_password->mPassword );
	QWidget::setTabOrder( d->ui->m_password->mPassword, d->ui->m_autologin );
}

QQEditAccountWidget::~QQEditAccountWidget()
{
	delete d;
}

Kopete::Account * QQEditAccountWidget::apply()
{
	if ( !account() )
		setAccount( new QQAccount( d->protocol, d->ui->m_login->text() ) );
	
	KConfigGroup *config=account()->configGroup();

	account()->setExcludeConnect( d->ui->m_autologin->isChecked() );
	d->ui->m_password->save( &static_cast<QQAccount *>(account())->password() );

	config->writeEntry( "exportCustomPicture", d->ui->m_useDisplayPicture->isChecked() );
	if (d->ui->optionOverrideServer->isChecked() ) {
		config->writeEntry( "serverName", d->ui->m_serverName->text() );
		config->writeEntry( "serverPort", d->ui->m_serverPort->value()  );
	}
	else {
		config->writeEntry( "serverName", "tcpconn.tencent.com" );
		config->writeEntry( "serverPort", "80" );
	}

	config->writeEntry( "useHttpMethod", d->ui->optionUseHttpMethod->isChecked() );

	// Global Identity
	config->writeEntry( "ExcludeGlobalIdentity", d->ui->m_globalIdentity->isChecked() );

	// Save the avatar image
	if( d->ui->m_useDisplayPicture->isChecked() && !d->pictureData.isNull() )
	{
		d->pictureUrl = KStandardDirs::locateLocal( "appdata", "qqpicture-" +
				account()->accountId().toLower().replace( QRegExp("[./~]" ), "-" ) + ".png" );
		if ( d->pictureData.save( d->pictureUrl, "PNG" ) )
		{
			// static_cast<QQAccount *>( account() )->setPictureUrl( d->pictureUrl );
		}
		else
		{
			KMessageBox::sorry( this, i18n( "<qt>An error occurred when trying to change the display picture.<br>"
					"Make sure that you have selected a correct image file</qt>" ), i18n( "QQ Plugin" ) );
		}
	}

	// static_cast<QQAccount *>( account() )->resetPictureObject();

	if ( account()->isConnected() )
	{
		QQContact *myself = static_cast<QQContact *>( account()->myself() );
		QQNotifySocket *notify = static_cast<QQAccount *>( account() )->notifySocket();
		if ( d->ui->m_nickName->text() != myself->property( Kopete::Global::Properties::self()->nickName()).value().toString() )
		;
			// static_cast<QQAccount *>( account() )->setPublicName( d->ui->m_displayName->text() );

		if ( notify )
		{
		/*
			if ( d->ui->m_phw->text() != myself->phoneWork() && ( !d->ui->m_phw->text().isEmpty() || !myself->phoneWork().isEmpty() ) )
				notify->changePhoneNumber( "PHW", d->ui->m_phw->text() );
			if( d->ui->m_phh->text() != myself->phoneHome() && ( !d->ui->m_phh->text().isEmpty() || !myself->phoneHome().isEmpty() ) )
				notify->changePhoneNumber( "PHH", d->ui->m_phh->text() );
			if( d->ui->m_phm->text() != myself->phoneMobile() && ( !d->ui->m_phm->text().isEmpty() || !myself->phoneMobile().isEmpty() ) )
				notify->changePhoneNumber( "PHM", d->ui->m_phm->text() );
			// (the && .isEmpty is because one can be null and the other empty)

			if ( ( config->readEntry("BLP") == "BL" ) != d->ui->m_blp->isChecked() )
			{
				// Yes, I know, calling sendCommand here is not very clean - Olivier
				notify->sendCommand( "BLP", d->ui->m_blp->isChecked() ? "BL" : "AL" );
			}
		*/
		}
	}
	return account();
}

bool QQEditAccountWidget::validateData()
{
	QString userid = d->ui->m_login->text();
	//if ( QQProtocol::validContactId( userid ) )
	//	return true;

	KMessageBox::queuedMessageBox( Kopete::UI::Global::mainWidget(), KMessageBox::Sorry,
		i18n( "<qt>You must enter a valid email address.</qt>" ), i18n( "QQ Plugin" ) );
	return false;
}

void QQEditAccountWidget::slotOpenRegister()
{
	KToolInvocation::invokeBrowser( "http://qqx.qq.com/"  );
}

#include "qqeditaccountwidget.moc"

// vim: set noet ts=4 sts=4 sw=4:

