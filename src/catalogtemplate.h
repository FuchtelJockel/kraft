/***************************************************************************
      catalogtemplate - template base class for catalog data
                             -------------------
    begin                : Oct 2007
    copyright            : (C) 2007 by Klaas Freitag
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
#ifndef CATALOGTEMPLATE_H
#define CATALOGTEMPLATE_H

/**
 * base class that is the base for all templates in kraft catalogs.
 */
#include <qptrlist.h>

class QWidget;
class QListViewItem;
class CatalogSelection;
class Katalog;
class Geld;

class CatalogTemplate
{
public:
  typedef enum { Unknown, ManualPrice, Calculation, AutoCalc } CalculationType;

  CatalogTemplate();
  virtual bool save() = 0;

  virtual Geld unitPrice() = 0;

  CalculationType calcKind();
  void setCalculationType( CalculationType t );
  QString calcKindString() const ;
  int sortKey() { return mSortKey; }
  void setSortKey( int k ) { mSortKey = k; }
protected:
  CalculationType m_calcType;
  int mSortKey;
};

class CatalogTemplateList : public QPtrList<CatalogTemplate>
{
public:
  CatalogTemplateList();
  // virtual ~CatalogTemplateList();

protected:
  // int compareItems( QPtrCollection::Item, QPtrCollection::Item );
  virtual int compareItems( CatalogTemplate*, CatalogTemplate* );
};

// typedef QPtrList<CatalogTemplate> CatalogTemplateList;

#endif

