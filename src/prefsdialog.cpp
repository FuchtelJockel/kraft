/***************************************************************************
                   prefsdialog.cpp  - the preferences Dialog
                             -------------------
    begin                : Sun Jul 3 2004
    copyright            : (C) 2004 by Klaas Freitag
    email                : freitag@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <QLayout>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QToolTip>
#include <QPalette>
#include <QSqlTableModel>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QStackedWidget>
#include <QHeaderView>
#include <QDialog>
#include <QMessageBox>

#include <QDebug>
#include <QDialogButtonBox>
#include <QVBoxLayout>

#include "prefsdialog.h"
#include "prefswages.h"
#include "prefsunits.h"
#include "databasesettings.h"
#include "kraftsettings.h"
#include "kraftdb.h"
#include "kraftdoc.h"
#include "defaultprovider.h"
#include "doctype.h"
#include "doctypeedit.h"
#include "taxeditdialog.h"
#include "documentman.h"
#include "impviewwidgets.h"
#include "texttemplate.h"
#include "htmlview.h"
#include "addressselectordialog.h"


// ################################################################################

PrefsDialog::PrefsDialog( QWidget *parent)
    : KPageDialog( parent )
{
  setFaceType( KPageDialog::List );
  setModal( true );
  setWindowTitle( i18n( "Configure Kraft" ) );

  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
  QWidget *mainWidget = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout;
  setLayout(mainLayout);
  mainLayout->addWidget(mainWidget);
  QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
  okButton->setDefault(true);
  okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
  connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
  mainLayout->addWidget(buttonBox);
  okButton->setDefault(true);
  setMinimumWidth(700);

  docTab();
  doctypeTab();
  taxTab();
  wagesTab();
  unitsTab();
  whoIsMeTab();

  readConfig();
}

void PrefsDialog::taxTab()
{
  QWidget *topWidget = new QWidget;

  KPageWidgetItem *topFrame = addPage( topWidget, i18n( "Taxes" ));

  topFrame->setIcon(QIcon::fromTheme( "accessories-text-editor" ) );

  QVBoxLayout *vboxLay = new QVBoxLayout;
  // vboxLay->setSpacing( spacingHint() );

  QLabel *label;
  label = new QLabel(i18n("Tax rates beginning at date:"));
  vboxLay->addWidget( label );

  mTaxModel = new QSqlTableModel(this);
  mTaxModel->setTable("taxes");
  mTaxModel->setSort(3, Qt::DescendingOrder);
  mTaxModel->setEditStrategy(QSqlTableModel::OnManualSubmit);
  mTaxModel->select();
  mTaxModel->setHeaderData(0, Qt::Horizontal, tr("ID"));
  mTaxModel->setHeaderData(1, Qt::Horizontal, tr("Full Tax [%]"));
  mTaxModel->setHeaderData(2, Qt::Horizontal, tr("Reduced Tax [%]"));
  mTaxModel->setHeaderData(3, Qt::Horizontal, tr("Start Date"));

  mTaxTreeView = new ImpTreeView;
  vboxLay->addWidget( mTaxTreeView );
  mTaxTreeView->setModel(mTaxModel);
  mTaxTreeView->setItemDelegate(new TaxItemDelegate());
  mTaxTreeView->hideColumn(0);
  mTaxTreeView->header()->moveSection(3, 1);
  mTaxTreeView->header()->stretchLastSection();
  mTaxTreeView->setColumnWidth(3, 200);
  mTaxTreeView->resizeColumnToContents(2);
  mTaxTreeView->resizeColumnToContents(1);

  connect( mTaxTreeView, SIGNAL(clicked(QModelIndex)),
           SLOT( slotTaxSelected(QModelIndex) ) );

  QHBoxLayout *butLay = new QHBoxLayout;
  butLay->addStretch( 1 );

  QPushButton *but = new QPushButton( QIcon::fromTheme("list-add"), i18n( "Add" ));
  connect( but, SIGNAL( clicked() ), SLOT( slotAddTax() ) );
  butLay->addWidget( but );

  mDelTax = new QPushButton( QIcon::fromTheme("list-remove"), i18n( "Remove" ) );
  connect( mDelTax, SIGNAL( clicked() ), SLOT( slotDeleteTax() ) );
  butLay->addWidget( mDelTax );
  mDelTax->setEnabled( false );

  vboxLay->addLayout( butLay );
  topWidget->setLayout( vboxLay );
}

void PrefsDialog::wagesTab()
{
    mPrefsWages = new PrefsWages(this);

    KPageWidgetItem *topFrame = addPage( mPrefsWages, i18n( "Wages" ));

    topFrame->setIcon(QIcon::fromTheme( "help-donate" ) );
}

void PrefsDialog::unitsTab()
{
    mPrefsUnits = new PrefsUnits(this);

    KPageWidgetItem *topFrame = addPage( mPrefsUnits, i18n( "Units" ));

    topFrame->setIcon(QIcon::fromTheme( "chronometer" ) );
}

void PrefsDialog::whoIsMeTab()
{
  QWidget *topWidget = new QWidget;

  KPageWidgetItem *topFrame = addPage( topWidget, i18n( "Own Identity" ));

  topFrame->setIcon(QIcon::fromTheme( "user-identity" ) );

  QVBoxLayout *vboxLay = new QVBoxLayout;
  // vboxLay->setSpacing( spacingHint() );

  QLabel *label;
  label = new QLabel(i18n("Select the identity of the sending entity of documents. That's <b>your companies</b> address."));
  label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  vboxLay->addWidget( label );

  QVBoxLayout *butLay = new QVBoxLayout;
  butLay->addStretch( 1 );

  mIdentityView = new HtmlView;
  QString home = QString::fromLatin1(qgetenv("KRAFT_HOME"));
  QString idFile = QString("%1/reports/images/identity.png").arg(home);
  QFileInfo fi(idFile);

  if(! fi.exists() ) {
      QString idFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, "kraft/reports/images/identity.png" );
      fi.setFile(idFile);
      fi.refresh();

      if( fi.exists() ) {
          idFile = fi.path();
      }
  } else {
      idFile = fi.path();
  }
  mIdentityView->setBaseUrl(idFile);

  butLay->addWidget(mIdentityView);
  _pbChangeIdentity = new QPushButton(i18n("Select Identity..."));
  connect( _pbChangeIdentity, SIGNAL(clicked()), SLOT(slotChangeIdentity()) );
  butLay->addWidget(_pbChangeIdentity);

  vboxLay->addLayout( butLay );

  topWidget->setLayout( vboxLay );

}

void PrefsDialog::slotChangeIdentity()
{
  AddressSelectorDialog dialog(this);

  if( dialog.exec() ) {
    Addressee identity = dialog.addressee();
    if( ! identity.isEmpty() ) {
      setMyIdentity(identity, true);
      emit newOwnIdentity(identity.uid(), identity);
    }
  }
}

void PrefsDialog::slotAddTax()
{
  TaxEditDialog *dialog = new TaxEditDialog(mTaxModel, this);
  dialog->show();
}

void PrefsDialog::slotDeleteTax()
{
  if ( mTaxTreeView->currentIndex().isValid() )
  {
    int row = mTaxTreeView->currentIndex().row();
    //mTaxTreeView->setRowHidden( row, mTaxTreeView->rootIndex(), true );
    mTaxModel->removeRows(row, 1);
  }
}

void PrefsDialog::slotTaxSelected(QModelIndex)
{
  bool state = false;
  if ( mTaxTreeView->currentIndex().isValid() ) {
    state = true;
  }

  mDelTax->setEnabled( state );
}

void PrefsDialog::docTab()
{
  QLabel *label;
  QWidget *topWidget = new QWidget;

  KPageWidgetItem *topFrame = addPage( topWidget, i18n( "Document Defaults" ) );
  topFrame->setIcon(QIcon::fromTheme( "edit-copy" ) );

  QVBoxLayout *vboxLay = new QVBoxLayout;
  topWidget->setLayout( vboxLay );
  QGridLayout *topLayout = new QGridLayout;
  vboxLay->addLayout( topLayout );

  label = new QLabel(i18n("&Default document type on creation:") );
  topLayout->addWidget(label, 0,0);

  mCbDocTypes = new QComboBox;
  label->setBuddy( mCbDocTypes );
  mCbDocTypes->setToolTip( i18n( "New documents default to the selected type." ) );
  topLayout->addWidget( mCbDocTypes, 0, 1 );
  mCbDocTypes->insertItems(-1, DocType::allLocalised() );

  QLabel *f = new QLabel;
  f->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  vboxLay->addWidget( f );

  // Localisation on document level
  mCbDocLocale = new QCheckBox( i18n( "Enable &Localization on Document Level" ) );
  mCbDocLocale->setToolTip( i18n( "Checking this enables language settings for each document."
                                     "<br>Leave it unchecked to use the KDE Workspace default settings for "
                                     "the document localisation." ) );
  vboxLay->addWidget( mCbDocLocale );

  vboxLay->addWidget( new QWidget );

  f = new QLabel;
  f->setFrameStyle( QFrame::HLine | QFrame::Sunken );
  vboxLay->addWidget( f );

  QHBoxLayout *butLay = new QHBoxLayout;
  QLabel *l = new QLabel( i18n( "Default &Tax for Documents:" ) );
  butLay->addWidget( l );
  mCbDefaultTaxType = new QComboBox;
  butLay->addWidget( mCbDefaultTaxType );
  l->setBuddy( mCbDefaultTaxType );

  mCbDefaultTaxType->setToolTip( i18n( "The default tax setting for all documents." ) );
  mCbDefaultTaxType->insertItem( 0, i18n("Display no tax at all" , 0));
  mCbDefaultTaxType->insertItem( 1, i18n("Calculate reduced tax for all items" ));
  mCbDefaultTaxType->insertItem( 2, i18n("Calculate full tax for all items" ) );
  // mCbDefaultTaxType->insertItem( 3, i18n("Calculate on individual item tax rate" ));
  vboxLay->addLayout( butLay );

  // space eater
  QWidget *spaceEater = new QWidget;
  spaceEater->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
  vboxLay->addWidget( spaceEater );
}

void PrefsDialog::doctypeTab()
{
  QWidget *topWidget = new QWidget;
  KPageWidgetItem *topFrame = addPage( topWidget, i18n( "Document Types" ) );
  topFrame->setIcon(QIcon::fromTheme( "folder-documents" ) );

  QVBoxLayout *vboxLay = new QVBoxLayout;
  topWidget->setLayout(vboxLay);
  vboxLay->setSpacing( 0 ); // spacingHint() );

  mDocTypeEdit = new DocTypeEdit;
  vboxLay->addWidget( mDocTypeEdit );

  connect( mDocTypeEdit, SIGNAL( removedType( const QString& ) ),
           SLOT( slotDocTypeRemoved( const QString& ) ) );

}

void PrefsDialog::slotDocTypeRemoved( const QString& type )
{
  // check if the default document type is still there
  QString currDefault = mCbDocTypes->currentText();

  if ( currDefault == type ) {
      QMessageBox msgBox;
      msgBox.setText(i18n( "The old default doc type for new documents was just deleted."
                           "Please check the setting in the Document Defaults in the "
                           "Kraft preferences Dialog." ));
      msgBox.setInformativeText(i18n("Document Default Change"));
      msgBox.setStandardButtons(QMessageBox::Ok);
      msgBox.setDefaultButton(QMessageBox::Ok);
      msgBox.exec();
  }


  for ( int i=0; i < mCbDocTypes->count(); i++ ) {
    if ( mCbDocTypes->itemText( i ) == type ) {
      mCbDocTypes->removeItem( i );
      continue;
    }
  }
}

void PrefsDialog::readConfig()
{
    mCbDocLocale->setChecked( KraftSettings::self()->showDocumentLocale() );

    QString t = KraftSettings::self()->doctype();
    if ( t.isEmpty() ) t = DefaultProvider::self()->docType();

    mCbDocTypes->setCurrentIndex( mCbDocTypes->findText( t ));

    mCbDefaultTaxType->setCurrentIndex( KraftSettings::self()->defaultTaxType()-1 );
}

void PrefsDialog::writeConfig()
{
    KraftSettings::self()->setShowDocumentLocale( mCbDocLocale->isChecked() );
    KraftSettings::self()->setDoctype( mCbDocTypes->currentText() );
    KraftSettings::self()->setDefaultTaxType( 1+mCbDefaultTaxType->currentIndex() );

    KraftSettings::self()->save();
}

void PrefsDialog::writeTaxes()
{
    mTaxModel->submitAll();
}

PrefsDialog::~PrefsDialog()
{
}

void PrefsDialog::accept()
{
  mDocTypeEdit->saveDocTypes();
  mPrefsWages->save();
  mPrefsUnits->save();
  writeTaxes();
  writeConfig();
  QDialog::accept();
}

#define IDENTITY_TAG(X) QLatin1String(X)
#define QL1(X) QLatin1String(X)

void PrefsDialog::setMyIdentity( const KContacts::Addressee& addressee, bool backendUp )
{
  // Note: This code is stolen from DocDigestDetailView::slotShowDocDetails
  // It should be refactored.

  TextTemplate tmpl("identity.trml");
  if( !tmpl.open() ) {
      return;
  }
  if( ! tmpl.errorString().isEmpty() ) {
    mIdentityView->displayContent( QString("<h1>Unable to find template <i>identity.trml</i></h1><p>%1</p>")
                                   .arg(tmpl.errorString()));
    return;
  }

  QString addressBookInfo;
  _pbChangeIdentity->setEnabled(backendUp);
  if( !backendUp ) {
      addressBookInfo = i18n("The identity can not be found.");
      tmpl.createDictionary(QL1("NO_IDENTITY"));
      tmpl.setValue(QL1("NO_IDENTITY_WRN"), i18n("<p><b>Kraft Addressbook Integration down.</b></p>"
                                             "<p>The address book backend is not up and running.</p>"
                                             "<p>Please check your addressbook integration setup.</p>"));
  } else {
      if( addressee.isEmpty() ) {
          addressBookInfo = i18n("The identity is not listed in an address book.");
          tmpl.createDictionary(QL1("NO_IDENTITY"));
          tmpl.setValue(QL1("NO_IDENTITY_WRN"), i18n("<p><b>Kraft does not know your identity.</b></p>"
                                                     "<p>Please pick one from the address books by clicking on the Button below.</p>"
                                                     "<p>Not having an identity selected can make your documents look incomplete.</p>"));
      } else {
          addressBookInfo  = i18n("Your identity can be found in the address books.");
          tmpl.createDictionary(QL1("IDENTITY"));
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_NAME"), addressee.realName() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_ORGANISATION"), addressee.organization() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_URL"), addressee.url().toString() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_EMAIL"), addressee.preferredEmail() );

          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_WORK_PHONE"), addressee.phoneNumber(PhoneNumber::Work).number());
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_MOBILE_PHONE"), addressee.phoneNumber(PhoneNumber::Cell).number());
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("IDENTITY_FAX"), addressee.phoneNumber(PhoneNumber::Fax).number());

          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("WORK_PHONE_LABEL"), i18n("Work Phone") );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("FAX_LABEL"), i18n("Fax") );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("MOBILE_PHONE_LABEL"), i18n("Cell Phone") );
          KContacts::Address myAddress;
          myAddress = addressee.address( KContacts::Address::Pref );
          QString addressType = i18n("preferred address");

          if( myAddress.isEmpty() ) {
              myAddress = addressee.address( KContacts::Address::Home );
              addressType = i18n("home address");
          }
          if( myAddress.isEmpty() ) {
              myAddress = addressee.address( KContacts::Address::Work );
              addressType = i18n("work address");
          }
          if( myAddress.isEmpty() ) {
              myAddress = addressee.address( KContacts::Address::Postal );
              addressType = i18n("postal address");
          }
          if( myAddress.isEmpty() ) {
              myAddress = addressee.address( KContacts::Address::Intl );
              addressType = i18n("international address");
          }
          if( myAddress.isEmpty() ) {
              myAddress = addressee.address( KContacts::Address::Dom );
              addressType = i18n("domestic address");
          }

          if( myAddress.isEmpty() ) {
              addressType = i18n("unknown");
              // qDebug () << "WRN: Address is still empty!";
          }

          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_POSTBOX" ),  myAddress.postOfficeBox() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_EXTENDED" ), myAddress.extended() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_STREET" ),   myAddress.street() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_LOCALITY" ), myAddress.locality() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_REGION" ),   myAddress.region() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_POSTCODE" ), myAddress.postalCode() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_COUNTRY" ),  myAddress.country() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_REGION" ),   myAddress.region() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_LABEL" ),    myAddress.label() );
          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG( "IDENTITY_ADDRESS_TYPE" ), QL1("(")+addressType+QL1(")") );

          tmpl.setValue( QL1("IDENTITY"), IDENTITY_TAG("ADDRESSBOOK_INFO"), addressBookInfo );
      }
  }

  const QString details = tmpl.expand();
  mIdentityView->displayContent( details );

}

TaxItemDelegate::TaxItemDelegate(QObject * parent) : QItemDelegate(parent) {}

void TaxItemDelegate::paint ( QPainter * painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const
{
  if(index.column() == 1 || index.column() == 2)
  {
    double percentage = index.data(Qt::DisplayRole).toDouble();
    // QString string = DefaultProvider::self()->locale()->formatNumber(QString::number(percentage), true, 1);
    QString string = DefaultProvider::self()->locale()->toString(percentage);
    drawDisplay(painter, option, option.rect, string);
  }
  else if(index.column() == 3)
  {
    QDate date = index.data(Qt::DisplayRole).toDate();
    // QString string = DefaultProvider::self()->locale()->formatDate(date);
    QString string = DefaultProvider::self()->locale()->toString(date);
    drawDisplay(painter, option, option.rect, string);
  }
  else
  {
    QItemDelegate::paint(painter, option, index);
  }
}
