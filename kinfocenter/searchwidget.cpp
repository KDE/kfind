/*
  Copyright (c) 2000 Matthias Elter <elter@kde.org>
  Copyright (c) 2004 Daniel Molkentin <molkentin@kde.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include <Q3PtrList>

#include <QLabel>
#include <QLayout>
#include <QListWidgetItem>
#include <QRegExp>
#include <QVBoxLayout>

#include <KApplication>
#include <klineedit.h>
#include <kiconloader.h>
#include <klocale.h>
#include <klistwidget.h>

#include "searchwidget.h"
#include "searchwidget.moc"

/**
 * Helper class for sorting icon modules by name without losing the fileName ID
 */
class ModuleItem : public QListWidgetItem
{
public:
 ModuleItem(ConfigModule *module, QListWidget * listbox = 0) :
	QListWidgetItem(listbox)
  , m_module(module)
 {
    setText( module->moduleName() ); 
    setIcon( KIconLoader::global()->loadIcon(module->icon(), KIconLoader::Desktop, KIconLoader::SizeSmall) ); 
 }

 ConfigModule *module() const { return m_module; }

protected:
 ConfigModule *m_module;

};

KeywordListEntry::KeywordListEntry(const QString& name, ConfigModule* module)
  : _name(name)
{
  if(module)
    _modules.append(module);
}

void KeywordListEntry::addModule(ConfigModule* module)
{
  if(module)
    _modules.append(module);
}

SearchWidget::SearchWidget(QWidget *parent)
  : QWidget(parent)
{
  _keywords.setAutoDelete(true);

  QVBoxLayout * l = new QVBoxLayout(this);
  l->setMargin(0);
  l->setSpacing(2);

  // input
  _input = new KLineEdit(this);
  _input->setFocus();
  QLabel *inputl = new QLabel(i18n("Se&arch:"), this);
  inputl->setBuddy(_input);

  l->addWidget(inputl);
  l->addWidget(_input);

  // keyword list
  _keyList = new KListWidget(this);
  QLabel *keyl = new QLabel(i18n("&Keywords:"), this);
  keyl->setBuddy(_keyList);

  l->addWidget(keyl);
  l->addWidget(_keyList);

  // result list
  _resultList = new KListWidget(this);
  QLabel *resultl = new QLabel(i18n("&Results:"), this);
  resultl->setBuddy(_resultList);

  l->addWidget(resultl);
  l->addWidget(_resultList);

  // set stretch factors
  l->setStretchFactor(_resultList, 1);
  l->setStretchFactor(_keyList, 2);


  connect(_input, SIGNAL(textChanged(const QString&)),
          this, SLOT(slotSearchTextChanged(const QString&)));

  connect(_keyList, SIGNAL(highlighted(const QString&)),
          this, SLOT(slotKeywordSelected(const QString&)));

  connect(_resultList, SIGNAL(itemChanged (QListWidgetItem*)),
          this, SLOT(slotModuleSelected(QListWidgetItem *)));
  connect(_resultList, SIGNAL(executed(QListWidgetItem *)),
          this, SLOT(slotModuleClicked(QListWidgetItem *)));
}

void SearchWidget::populateKeywordList(ConfigModuleList *list)
{
  ConfigModule *module;

  // loop through all control modules
  for (module=list->first(); module != 0; module=list->next())
    {
      if (module->library().isEmpty())
        continue;

      // get the modules keyword list
      QStringList kw = module->keywords();

      // loop through the keyword list to populate _keywords
      for(QStringList::ConstIterator it = kw.begin(); it != kw.end(); ++it)
        {
          QString name = (*it).toLower();
          bool found = false;

          // look if _keywords already has an entry for this keyword
          for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
            {
              // if there is an entry for this keyword, add the module to the entries module list
              if (k->moduleName() == name)
                {
                  k->addModule(module);
                  found = true;
                  break;
                }
            }

          // if there is entry for this keyword, create a new one
          if (!found)
            {
              KeywordListEntry *k = new KeywordListEntry(name, module);
              _keywords.append(k);
            }
        }
    }
  populateKeyListBox("*");
}

void SearchWidget::populateKeyListBox(const QString& s)
{
  _keyList->clear();

  QStringList matches;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if ( QRegExp(s, Qt::CaseInsensitive, QRegExp::Wildcard).indexIn(k->moduleName()) >= 0)
        matches.append(k->moduleName().trimmed());
    }

  for(QStringList::ConstIterator it = matches.begin(); it != matches.end(); ++it)
    _keyList->addItem(*it);

  _keyList->model()->sort(0);
}

void SearchWidget::populateResultListBox(const QString& s)
{
  _resultList->clear();

  Q3PtrList<ModuleItem> results;

  for(KeywordListEntry *k = _keywords.first(); k != 0; k = _keywords.next())
    {
      if (k->moduleName() == s)
        {
          Q3PtrList<ConfigModule> modules = k->modules();

          for(ConfigModule *m = modules.first(); m != 0; m = modules.next())
              new ModuleItem(m, _resultList);
        }
    }

  _resultList->model()->sort(0);
}

void SearchWidget::slotSearchTextChanged(const QString & s)
{
  QString regexp = s;
  regexp += '*';
  populateKeyListBox(regexp);
}

void SearchWidget::slotKeywordSelected(const QString & s)
{
  populateResultListBox(s);
}

void SearchWidget::slotModuleSelected(QListWidgetItem *item)
{
  if (item)
    emit moduleSelected( static_cast<ModuleItem*>(item)->module() );
}

void SearchWidget::slotModuleClicked(QListWidgetItem *item)
{
  if (item)
    emit moduleSelected( static_cast<ModuleItem*>(item)->module() );
}
