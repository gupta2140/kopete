/*
 * This file is part of Kopete
 *
 * Copyright (C) 2009 Collabora Ltd. <info@collabora.co.uk>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "ui/telepathyeditaccountwidget.h"

#include "telepathyprotocol.h"

#include <telepathyaccount.h>
#include <telepathyprotocolinternal.h>

#include <KCMTelepathyAccounts/AbstractAccountUi>
#include <KCMTelepathyAccounts/AbstractAccountUi>
#include <KCMTelepathyAccounts/ConnectionManagerItem>
#include <KCMTelepathyAccounts/MandatoryParameterEditWidget>
#include <KCMTelepathyAccounts/OptionalParameterEditWidget>
#include <KCMTelepathyAccounts/PluginManager>
#include <KCMTelepathyAccounts/ProtocolItem>
#include <KCMTelepathyAccounts/ProtocolSelectWidget>

#include <KDebug>
#include <KGlobal>
#include <KLocale>
#include <KSharedConfig>
#include <KTabWidget>

#include <kopeteaccount.h>

#include <QtGui/QGridLayout>

#include <TelepathyQt4/ConnectionManager>
#include <TelepathyQt4/PendingStringList>

class TelepathyEditAccountWidget::Private
{
public:
    Private()
     : tabWidget(0),
       protocolSelectWidget(0),
       mandatoryParametersWidget(0),
       mainLayout(0)
    {
        kDebug();
    }

    KTabWidget *tabWidget;
    ProtocolSelectWidget *protocolSelectWidget;
    AbstractAccountParametersWidget *mandatoryParametersWidget;
    QList<AbstractAccountParametersWidget*> optionalParametersWidgets;
    QString mandatoryPageDesc;
    QList<QString> optionalPageDesc;
    QGridLayout *mainLayout;
};

TelepathyEditAccountWidget::TelepathyEditAccountWidget(Kopete::Account *account, QWidget *parent)
 : QWidget(parent),
   KopeteEditAccountWidget(account),
   d(new Private)
{
    kDebug();

    // When account is 0, we are creating a new account.
    if (!account) {
        // Add account UI
        setupAddAccountUi();
    } else {
        // Edit account UI
        setupEditAccountUi();
    }
}

TelepathyEditAccountWidget::~TelepathyEditAccountWidget()
{
    kDebug();

    delete d;
}

TelepathyAccount *TelepathyEditAccountWidget::account()
{
    kDebug();

    return qobject_cast<TelepathyAccount*>(KopeteEditAccountWidget::account());
}

bool TelepathyEditAccountWidget::validateData()
{
    kDebug();

    // Check there is a mandatory parameters widget. If not, then fail.
    if (!d->mandatoryParametersWidget) {
        return false;
    }

    // Check all widgets validate OK.
    if (!d->mandatoryParametersWidget->validateParameterValues()) {
        kDebug() << "A widget failed parameter validation. Not accepting wizard.";
        return false;
    }

    foreach (AbstractAccountParametersWidget *w, d->optionalParametersWidgets) {
        if (!w->validateParameterValues()) {
            kDebug() << "A widget failed parameter validation. Not accepting wizard.";
            return false;
        }
    }

    return true;
}

Kopete::Account *TelepathyEditAccountWidget::apply()
{
    kDebug();

    // Check if we are applying an added account or edited account.
    if (account()) {
        // Edited an account.
        return applyEditedAccount();
    }

    // Added an account.
    return applyAddedAccount();
}

Kopete::Account *TelepathyEditAccountWidget::applyAddedAccount()
{
    kDebug();
    
    // Get the mandatory parameters.
    QMap<Tp::ProtocolParameter*, QVariant> mandatoryParameterValues;
    mandatoryParameterValues = d->mandatoryParametersWidget->parameterValues();

    // Get the optional properties
    QMap<Tp::ProtocolParameter*, QVariant> optionalParameterValues;

    foreach (AbstractAccountParametersWidget *w, d->optionalParametersWidgets) {
        QMap<Tp::ProtocolParameter*, QVariant> parameters = w->parameterValues();
        QMap<Tp::ProtocolParameter*, QVariant>::const_iterator i = parameters.constBegin();
        while (i != parameters.constEnd()) {
            if (!optionalParameterValues.contains(i.key())) {
                optionalParameterValues.insert(i.key(), i.value());
            } else {
                kWarning() << "Parameter:" << i.key()->name() << "is already in the map.";
            }

            ++i;
        }
        continue;
    }

    // Get the ProtocolItem that was selected and the corresponding ConnectionManagerItem.
    ProtocolItem *protocolItem = d->protocolSelectWidget->selectedProtocol();
    ConnectionManagerItem *connectionManagerItem = qobject_cast<ConnectionManagerItem*>(protocolItem->parent());

    if (!connectionManagerItem) {
        kWarning() << "Invalid ConnectionManager item.";
        return 0;
    }

    // Merge the parameters into a QVariantMap for submitting to the Telepathy AM.
    QVariantMap parameters;


    foreach (Tp::ProtocolParameter *pp, mandatoryParameterValues.keys()) {
        QVariant value = mandatoryParameterValues.value(pp);

        // Don't try and add empty parameters or ones where the default value is still set.
        if ((!value.isNull()) && (value != pp->defaultValue())) {

            // Check for params where they are empty and the default is null.
            if (pp->type() == QVariant::String) {
                if ((pp->defaultValue() == QVariant()) && (value.toString().isEmpty())) {
                    continue;
                }
            }

            parameters.insert(pp->name(), value);
        }
    }

    foreach (Tp::ProtocolParameter *pp, optionalParameterValues.keys()) {
        QVariant value = optionalParameterValues.value(pp);

        kDebug() << "Op Param:" << pp->name() << value;

        // Don't try and add empty parameters or ones where the default value is still set.
        if ((!value.isNull()) && (value != pp->defaultValue())) {

            // Check for params where they are empty and the default is null.
            if (pp->type() == QVariant::String) {
                if ((pp->defaultValue() == QVariant()) && (value.toString().isEmpty())) {
                    continue;
                }
            }

            parameters.insert(pp->name(), value);
        }
    }

    setAccount(TelepathyProtocol::protocol()->createNewAccount(parameters.value("account").toString()));

    writeConfig(connectionManagerItem->connectionManager()->name(),
                protocolItem->protocol(),
                parameters);

    return account();
}

Kopete::Account *TelepathyEditAccountWidget::applyEditedAccount()
{
    kDebug();

    // Use the code from KCM to get the parameters and values and unset ones.
    // Get the mandatory parameters.
    QMap<Tp::ProtocolParameter*, QVariant> mandatoryParameterValues;
    mandatoryParameterValues = d->mandatoryParametersWidget->parameterValues();

    // Get the optional parameters.
    QMap<Tp::ProtocolParameter*, QVariant> optionalParameterValues;

    foreach (AbstractAccountParametersWidget *w, d->optionalParametersWidgets) {
        QMap<Tp::ProtocolParameter*, QVariant> parameters = w->parameterValues();
        QMap<Tp::ProtocolParameter*, QVariant>::const_iterator i = parameters.constBegin();
        while (i != parameters.constEnd()) {
            if (!optionalParameterValues.contains(i.key())) {
                optionalParameterValues.insert(i.key(), i.value());
            } else {
                kWarning() << "Parameter:" << i.key()->name() << "is already in the map.";
            }

            ++i;
        }
        continue;
    }

    // Merge the parameters into a QVariantMap for submitting to the Telepathy AM.
    QVariantMap parameters;
    QVariantMap allParameters;
    QStringList unsetParameters;

    foreach (Tp::ProtocolParameter *pp, mandatoryParameterValues.keys()) {
        QVariant value = mandatoryParameterValues.value(pp);

        allParameters.insert(pp->name(), value);

       // Unset null parameters.
        if (value.isNull()) {
            unsetParameters.append(pp->name());
            continue;
        }

        // Unset any parameters where the default value is equal to the current value.
        if (pp->defaultValue() == value) {
            unsetParameters.append(pp->name());
            continue;
        }

        // Unset any strings where the default is empty, and the value is an empty string
        if (pp->type() == QVariant::String) {
            if ((pp->defaultValue().isNull()) && value.toString().isEmpty()) {
                unsetParameters.append(pp->name());
                continue;
            }
        }

        // Parameter has a valid value, so set it.
        parameters.insert(pp->name(), value);
    }

    foreach (Tp::ProtocolParameter *pp, optionalParameterValues.keys()) {
        QVariant value = optionalParameterValues.value(pp);

        allParameters.insert(pp->name(), value);

       // Unset null parameters.
        if (value.isNull()) {
            unsetParameters.append(pp->name());
            continue;
        }

        // Unset any parameters where the default value is equal to the current value.
        if (pp->defaultValue() == value) {
            unsetParameters.append(pp->name());
            continue;
        }

        // Unset any strings where the default is empty, and the value is an empty string
        if (pp->type() == QVariant::String) {
            if ((pp->defaultValue().isNull()) && value.toString().isEmpty()) {
                unsetParameters.append(pp->name());
                continue;
            }
        }

        // Parameter has a valid value, so set it.
        parameters.insert(pp->name(), value);
    }

    // Write the kopete config file.
    writeConfig(account()->account()->cmName(), account()->account()->protocol(), allParameters);

    // Tell the account to update with these parameters.
    account()->accountEdited(parameters, unsetParameters);

    return account();
}

void TelepathyEditAccountWidget::writeConfig(const QString &connectionManager,
                                             const QString &protocol,
                                             const QVariantMap &parameters)
{
    kDebug();

    QString accountId = account()->accountId();

    // Write basic account configuration
    KConfigGroup *basicConfig = account()->configGroup();
    basicConfig->writeEntry(QLatin1String("ConnectionManager"), connectionManager);
    basicConfig->writeEntry(QLatin1String("TelepathyProtocol"), protocol);

    // Write config related to ConnectionManager Parameters
    KConfigGroup parametersConfig = 
            KGlobal::config()->group(TelepathyProtocolInternal::protocolInternal()->
                                     formatTelepathyConfigGroup(connectionManager, 
                                                                protocol, 
                                                                accountId));

    QVariantMap::const_iterator i = parameters.constBegin();
    while (i != parameters.constEnd()) {
        parametersConfig.writeEntry(i.key(), i.value());
        ++i;
    }
}

void TelepathyEditAccountWidget::setupAddAccountUi()
{
    kDebug();

    // Set up the Add Account UI.
    d->tabWidget = new KTabWidget(this);
    d->mainLayout = new QGridLayout(this);
    d->mainLayout->addWidget(d->tabWidget);

    d->protocolSelectWidget = new ProtocolSelectWidget(d->tabWidget);
    d->tabWidget->addTab(d->protocolSelectWidget, i18n("Select Protocol"));

    connect(d->protocolSelectWidget, SIGNAL(protocolGotSelected(bool)),
            SLOT(onProtocolGotSelected(bool)));
}

void TelepathyEditAccountWidget::setupEditAccountUi()
{
    kDebug();

    // Set up the Edit Account UI.
    d->tabWidget = new KTabWidget(this);
    d->mainLayout = new QGridLayout(this);
    d->mainLayout->addWidget(d->tabWidget);
    resize(400, 480);

    if(!account()->account())
    {
        kDebug() << "Account not exist!";
	return;
    }

    // Get the protocol's parameters.
    Tp::ProtocolInfo *protocolInfo = account()->account()->protocolInfo();
    Tp::ProtocolParameterList protocolParameters = protocolInfo->parameters();

    Tp::ProtocolParameterList optionalProtocolParameters;
    Tp::ProtocolParameterList mandatoryProtocolParameters;

    foreach (Tp::ProtocolParameter *parameter, protocolParameters) {
        if (parameter->isRequired()) {
            mandatoryProtocolParameters.append(parameter);
        } else {
            optionalProtocolParameters.append(parameter);
        }
    }

    // Get the parameter values.
    QVariantMap parameterValues = account()->account()->parameters();

    // Get the AccountsUi for the plugin, and get the optional parameter widgets for it.
    AbstractAccountUi *ui = PluginManager::instance()->accountUiForProtocol(account()->account()->cmName(),
                                                                            account()->account()->protocol());

    if (account()->account()->cmName() == "gabble") {
        kDebug() << "GABBLE!!!!!!!!!!!!!!!!!!!!!!!!!111!!!!!one!!!1!!1!";
        d->mandatoryPageDesc = i18n("Basic setup");
        d->optionalPageDesc.push_back(i18n("Account preferences"));
        d->optionalPageDesc.push_back(i18n("Connection settings"));
        d->optionalPageDesc.push_back(i18n("Advanced options"));
    }

    // Set up the Mandatory Parameters page
    Tp::ProtocolParameterList mandatoryParametersLeft = mandatoryProtocolParameters;

    // Create the custom UI or generic UI depending on available parameters.
    if (ui) {
        // UI does exist, set it up.
        AbstractAccountParametersWidget *widget = ui->mandatoryParametersWidget(mandatoryProtocolParameters,
                                                                                account()->account()->parameters());
        QMap<QString, QVariant::Type> manParams = ui->supportedMandatoryParameters();
        QMap<QString, QVariant::Type>::const_iterator manIter = manParams.constBegin();
        while(manIter != manParams.constEnd()) {
            foreach (Tp::ProtocolParameter *parameter, mandatoryProtocolParameters) {
                // If the parameter is not
                if ((parameter->name() == manIter.key()) &&
                    (parameter->type() == manIter.value())) {
                    mandatoryParametersLeft.removeAll(parameter);
                }
            }

            ++manIter;
        }

        if (mandatoryParametersLeft.isEmpty()) {
            d->mandatoryParametersWidget = widget;
        } else {
            // FIXME: At the moment, if the custom widget can't handle all the mandatory
            // parameters we fall back to the generic one for all of them. It might be nicer
            // to have the custom UI for as many as possible, and stick a small generic one
            // underneath for those parameters it doesn't handle.
            widget->deleteLater();
            widget = 0;
        }
    }

    if (!d->mandatoryParametersWidget) {
        d->mandatoryParametersWidget = new MandatoryParameterEditWidget(
                mandatoryProtocolParameters,
                account()->account()->parameters(),
                d->tabWidget);
    }

    if (d->mandatoryPageDesc.isEmpty()) {
        d->tabWidget->addTab(d->mandatoryParametersWidget, i18n("Mandatory Parameters"));
    } else {
        d->tabWidget->addTab(d->mandatoryParametersWidget, d->mandatoryPageDesc);
    }

    // Get the list of parameters
    Tp::ProtocolParameterList optionalParametersLeft = optionalProtocolParameters;

    int pageIndex = 0;

    // Check if the AbstractAccountUi exists. If not then we use the autogenerated UI for
    // everything.
    if (ui) {
        // UI Does exist, set it up.
        QList<AbstractAccountParametersWidget*> widgets = ui->optionalParametersWidgets(
                optionalProtocolParameters,
                account()->account()->parameters());

        // Remove all handled parameters from the optionalParameters list.
        QMap<QString, QVariant::Type> opParams = ui->supportedOptionalParameters();
        QMap<QString, QVariant::Type>::const_iterator opIter = opParams.constBegin();
        while(opIter != opParams.constEnd()) {
            foreach (Tp::ProtocolParameter *parameter, optionalProtocolParameters) {
                // If the parameter is not
                if ((parameter->name() == opIter.key()) &&
                    (parameter->type() == opIter.value())) {
                    optionalParametersLeft.removeAll(parameter);
                }
            }

            ++opIter;
        }

        foreach (AbstractAccountParametersWidget *widget, widgets) {
            d->optionalParametersWidgets.append(widget);
            int indexOf = widgets.indexOf(widget);
            QString description = i18n("Optional parameter");
            if (indexOf < d->optionalPageDesc.size()) {
                description = d->optionalPageDesc.at(indexOf);
            }

            d->tabWidget->addTab(widget, description);
            pageIndex++;
        }
    }

    // Show the generic UI if optionalParameters is not empty.
    if (optionalParametersLeft.size() > 0) {
        OptionalParameterEditWidget *opew =
                new OptionalParameterEditWidget(optionalParametersLeft,
                                                account()->account()->parameters(),
                                                d->tabWidget);
        d->optionalParametersWidgets.append(opew);
        if (pageIndex < d->optionalPageDesc.size()) {
            d->tabWidget->addTab(opew, d->optionalPageDesc.at(pageIndex));
        } else {
            d->tabWidget->addTab(opew, i18n("Optional Parameters"));
        }
    }
}

void TelepathyEditAccountWidget::onProtocolGotSelected(bool selected)
{
    kDebug();

    // If protocol was not selected, return.
    if (!selected) {
        kWarning() << "Protocol unselected.";
        return;
    }

    // Get the ProtocolItem and check it is valid.
    ProtocolItem *item = d->protocolSelectWidget->selectedProtocol();

    if (!item) {
        kWarning() << "Selected protocol is not valid.";
        return;
    }

    // Delete any existing parameters widgets
    if (d->mandatoryParametersWidget) {
        d->tabWidget->removePage(d->mandatoryParametersWidget);
        d->mandatoryParametersWidget->deleteLater();
        d->mandatoryParametersWidget = 0;
    }

    foreach (AbstractAccountParametersWidget *w, d->optionalParametersWidgets) {
        if (w) {
            d->tabWidget->removePage(w);
            w->deleteLater();
        }
    }
    d->optionalParametersWidgets.clear();

    ConnectionManagerItem *cmItem = qobject_cast<ConnectionManagerItem*>(item->parent());
    if (!cmItem) {
        kWarning() << "cmItem is invalid.";
    }

    QString connectionManager = cmItem->connectionManager()->name();
    QString protocol = item->protocol();

    kDebug() << connectionManager;
    
    if (connectionManager == "gabble") {
        kDebug() << "GABBLE!!!!!!!!!!!!!!!!!!!!!!!!!111!!!!!one!!!1!!1!";
        d->mandatoryPageDesc = i18n("Basic setup");
        d->optionalPageDesc.push_back(i18n("Account preferences"));
        d->optionalPageDesc.push_back(i18n("Connection settings"));
        d->optionalPageDesc.push_back(i18n("Advanced options"));
    }

    // Get the AccountsUi for the plugin, and get the optional parameter widgets for it.
    AbstractAccountUi *ui = PluginManager::instance()->accountUiForProtocol(connectionManager,
                                                                            protocol);

    // Set up the Mandatory Parameters page
    Tp::ProtocolParameterList mandatoryParameters = item->mandatoryParameters();
    Tp::ProtocolParameterList mandatoryParametersLeft = item->mandatoryParameters();

    // Create the custom UI or generic UI depending on available parameters.
    if (ui) {
        // UI does exist, set it up.
        AbstractAccountParametersWidget *widget = ui->mandatoryParametersWidget(mandatoryParameters);
        QMap<QString, QVariant::Type> manParams = ui->supportedMandatoryParameters();
        QMap<QString, QVariant::Type>::const_iterator manIter = manParams.constBegin();
        while(manIter != manParams.constEnd()) {
            foreach (Tp::ProtocolParameter *parameter, mandatoryParameters) {
                // If the parameter is not
                if ((parameter->name() == manIter.key()) &&
                    (parameter->type() == manIter.value())) {
                    mandatoryParametersLeft.removeAll(parameter);
                }
            }

            ++manIter;
        }

        if (mandatoryParametersLeft.isEmpty()) {
            d->mandatoryParametersWidget = widget;
        } else {
            // FIXME: At the moment, if the custom widget can't handle all the mandatory
            // parameters we fall back to the generic one for all of them. It might be nicer
            // to have the custom UI for as many as possible, and stick a small generic one
            // underneath for those parameters it doesn't handle.
            widget->deleteLater();
            widget = 0;
        }
    }

    if (!d->mandatoryParametersWidget) {
        d->mandatoryParametersWidget = new MandatoryParameterEditWidget(
                item->mandatoryParameters(), QVariantMap(), d->tabWidget);
    }

    if (d->mandatoryPageDesc.isEmpty()) {
        d->tabWidget->addTab(d->mandatoryParametersWidget, i18n("Mandatory Parameters"));
    } else {
        d->tabWidget->addTab(d->mandatoryParametersWidget, d->mandatoryPageDesc);
    }

    // Get the list of parameters
    Tp::ProtocolParameterList optionalParameters = item->optionalParameters();
    Tp::ProtocolParameterList optionalParametersLeft = item->optionalParameters();

    int pageIndex = 0;

    // Check if the AbstractAccountUi exists. If not then we use the autogenerated UI for
    // everything.
    if (ui) {
        // UI Does exist, set it up.
        QList<AbstractAccountParametersWidget*> widgets = ui->optionalParametersWidgets(optionalParameters);

        // Remove all handled parameters from the optionalParameters list.
        QMap<QString, QVariant::Type> opParams = ui->supportedOptionalParameters();
        QMap<QString, QVariant::Type>::const_iterator opIter = opParams.constBegin();
        while(opIter != opParams.constEnd()) {
            foreach (Tp::ProtocolParameter *parameter, optionalParameters) {
                // If the parameter is not
                if ((parameter->name() == opIter.key()) &&
                    (parameter->type() == opIter.value())) {
                    optionalParametersLeft.removeAll(parameter);
                }
            }

            ++opIter;
        }

        foreach (AbstractAccountParametersWidget *widget, widgets) {
            d->optionalParametersWidgets.append(widget);
            int indexOf = widgets.indexOf(widget);
            QString description = i18n("Optional parameter");
            if (indexOf < d->optionalPageDesc.size()) {
                description = d->optionalPageDesc.at(indexOf);
            }

            d->tabWidget->addTab(widget, description);
            pageIndex++;
        }
    }

    // Show the generic UI if optionalParameters is not empty.
    if (optionalParametersLeft.size() > 0) {
        OptionalParameterEditWidget *opew =
                new OptionalParameterEditWidget(optionalParametersLeft,
                                                QVariantMap(),
                                                d->tabWidget);
        d->optionalParametersWidgets.append(opew);
        if (pageIndex < d->optionalPageDesc.size()) {
            d->tabWidget->addTab(opew, d->optionalPageDesc.at(pageIndex));
        } else {
            d->tabWidget->addTab(opew, i18n("Optional Parameters"));
        }
    }
}


#include "telepathyeditaccountwidget.moc"
