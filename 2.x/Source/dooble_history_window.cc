/*
** Copyright (c) 2008 - present, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Dooble without specific prior written permission.
**
** DOOBLE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** DOOBLE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QClipboard>
#include <QDir>
#include <QKeyEvent>
#include <QMenu>
#include <QShortcut>
#include <QSqlQuery>

#include "dooble.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_history_window::dooble_history_window(void):QMainWindow()
{
  m_parent = 0;
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.period->setCurrentRow(0);
  m_ui.period->setStyleSheet("QListWidget {background-color: transparent;}");
  m_ui.splitter->setStretchFactor(0, 0);
  m_ui.splitter->setStretchFactor(1, 1);
  m_ui.splitter->restoreState
    (QByteArray::fromBase64(dooble_settings::
			    setting("history_window_splitter_state").
			    toByteArray()));
  m_ui.table->sortByColumn(0, Qt::AscendingOrder);
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(dooble::s_history,
	  SIGNAL(icon_updated(const QIcon &, const QUrl &)),
	  this,
	  SLOT(slot_icon_updated(const QIcon &, const QUrl &)));
  connect(dooble::s_history,
	  SIGNAL(item_updated(const QIcon &, const QWebEngineHistoryItem &)),
	  this,
	  SLOT(slot_item_updated(const QIcon &,
				 const QWebEngineHistoryItem &)));
  connect(dooble::s_history,
	  SIGNAL(new_item(const QIcon &, const QWebEngineHistoryItem &)),
	  this,
	  SLOT(slot_new_item(const QIcon &, const QWebEngineHistoryItem &)));
  connect(dooble::s_history,
	  SIGNAL(populated(void)),
	  this,
	  SLOT(slot_populate(void)));
  connect(m_ui.period,
	  SIGNAL(currentRowChanged(int)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));
  connect(m_ui.table,
	  SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_double_clicked(QTableWidgetItem *)));
  connect(this,
	  SIGNAL(customContextMenuRequested(const QPoint &)),
	  this,
	  SLOT(slot_show_context_menu(const QPoint &)));
  new QShortcut
    (QKeySequence(tr("Ctrl+F")), m_ui.search, SLOT(setFocus(void)));
  setContextMenuPolicy(Qt::CustomContextMenu);
}

void dooble_history_window::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_history_window::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_history_window::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_history_window::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("history_window_geometry", saveGeometry().toBase64());

  dooble_settings::set_setting
    ("history_window_splitter_state", m_ui.splitter->saveState().toBase64());
}

void dooble_history_window::set_row_hidden(int i)
{
  QTableWidgetItem *item1 = m_ui.table->item(i, 0);
  QTableWidgetItem *item2 = m_ui.table->item(i, 1);
  QTableWidgetItem *item3 = m_ui.table->item(i, 2);

  if(!item1 || !item2 || !item3)
    return;

  QDateTime period(QDateTime::currentDateTime());
  QString text(m_ui.search->text().toLower().trimmed());

  switch(m_ui.period->currentRow())
    {
    case 0: // All
      {
	if(text.isEmpty())
	  m_ui.table->setRowHidden(i, false);
	else if(item1->text().toLower().contains(text) ||
		item2->text().toLower().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    case 1: // Today
    case 2: // Yesterday
      {
	if(m_ui.period->currentRow() == 2)
	  period = period.addDays(-1);

	QDateTime dateTime
	  (QDateTime::fromString(item3->text(), Qt::ISODate));

	if(dateTime.date() == period.date())
	  {
	    if(text.isEmpty())
	      m_ui.table->setRowHidden(i, false);
	    else if(item1->text().toLower().contains(text) ||
		    item2->text().toLower().contains(text))
	      m_ui.table->setRowHidden(i, false);
	    else
	      m_ui.table->setRowHidden(i, true);
	  }
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    default:
      {
	if(m_ui.period->currentRow() == 4)
	  period = period.addMonths(-1);

	QDateTime dateTime
	  (QDateTime::fromString(item3->text(), Qt::ISODate));

	if(dateTime.date().month() == period.date().month() &&
	   dateTime.date().year() == period.date().year())
	  {
	    if(text.isEmpty())
	      m_ui.table->setRowHidden(i, false);
	    else if(item1->text().toLower().contains(text) ||
		    item2->text().toLower().contains(text))
	      m_ui.table->setRowHidden(i, false);
	    else
	      m_ui.table->setRowHidden(i, true);
	  }
	else
	  m_ui.table->setRowHidden(i, true);

	break;
      }
    }
}

void dooble_history_window::show(QWidget *parent)
{
  m_parent = parent;

  if(m_parent)
    {
      connect(m_parent,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(slot_parent_destroyed(void)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(open_url(const QUrl &)),
	      m_parent,
	      SLOT(slot_open_url(const QUrl &)),
	      Qt::UniqueConnection);
    }

  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("history_window_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble_history_window::showNormal(QWidget *parent)
{
  m_parent = parent;

  if(m_parent)
    {
      connect(m_parent,
	      SIGNAL(destroyed(void)),
	      this,
	      SLOT(slot_parent_destroyed(void)),
	      Qt::UniqueConnection);
      connect(this,
	      SIGNAL(open_url(const QUrl &)),
	      m_parent,
	      SLOT(slot_open_url(const QUrl &)),
	      Qt::UniqueConnection);
    }

  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("history_window_geometry").
					   toByteArray()));

  QMainWindow::showNormal();
}

void dooble_history_window::slot_copy_location(void)
{
  QClipboard *clipboard = QApplication::clipboard();

  if(!clipboard)
    return;

  QTableWidgetItem *item = m_ui.table->currentItem();

  if(!item)
    return;

  item = m_ui.table->item(item->row(), 1);

  if(item)
    clipboard->setText(item->text());
}

void dooble_history_window::slot_delete_pages(void)
{
  QList<QTableWidgetItem *> list(m_ui.table->selectedItems());

  if(list.isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    {
      for(int i = list.size() - 1; i >= 0; i--)
	{
	  if(!list.at(i))
	    continue;

	  QTableWidgetItem *item = m_ui.table->item(list.at(i)->row(), 0);

	  if(!item)
	    continue;

	  if(dooble::s_history)
	    dooble::s_history->remove_item(item->data(Qt::UserRole).toUrl());

	  list.removeAt(i);
	  m_items.remove(item->data(Qt::UserRole).toUrl());
	  m_ui.table->removeRow(item->row());
	}

      QApplication::restoreOverrideCursor();
      return;
    }

  QString database_name("dooble_history_window");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_history.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	for(int i = list.size() - 1; i >= 0; i--)
	  {
	    if(!list.at(i))
	      continue;

	    QTableWidgetItem *item = m_ui.table->item(list.at(i)->row(), 0);

	    if(!item)
	      continue;

	    QUrl url(item->data(Qt::UserRole).toUrl());

	    if(dooble::s_history)
	      dooble::s_history->remove_item(url);

	    query.prepare("DELETE FROM dooble_history WHERE url_digest = ?");
	    query.addBindValue
	      (dooble::s_cryptography->hmac(url.toEncoded()).toBase64());
	    query.exec();
	    list.removeAt(i);
	    m_items.remove(url);
	    m_ui.table->removeRow(item->row());
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_icon_updated(const QIcon &icon,
					      const QUrl &url)
{
  QTableWidgetItem *item = m_items.value(url);

  if(!item)
    return;

  if(icon.isNull())
    item->setIcon(dooble_favicons::icon(url));
  else
    item->setIcon(icon);
}

void dooble_history_window::slot_item_double_clicked(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(!m_parent)
    {
      /*
      ** Locate a Dooble window.
      */

      QWidgetList list(QApplication::topLevelWidgets());

      for(int i = 0; i < list.size(); i++)
	if(qobject_cast<dooble *> (list.at(i)))
	  {
	    m_parent = list.at(i);
	    connect(m_parent,
		    SIGNAL(destroyed(void)),
		    this,
		    SLOT(slot_parent_destroyed(void)),
		    Qt::UniqueConnection);
	    connect(this,
		    SIGNAL(open_url(const QUrl &)),
		    m_parent,
		    SLOT(slot_open_url(const QUrl &)),
		    Qt::UniqueConnection);
	    break;
	  }
    }

  emit open_url(item->data(Qt::UserRole).toUrl());
}

void dooble_history_window::slot_item_updated(const QIcon &icon,
					      const QWebEngineHistoryItem &item)
{
  if(!item.isValid())
    return;

  QTableWidgetItem *item1 = m_items.value(item.url());

  if(!item1)
    return;
  else
    {
      if(icon.isNull())
	item1->setIcon(dooble_favicons::icon(item.url()));
      else
	item1->setIcon(icon);
    }

  QTableWidgetItem *item2 = m_ui.table->item(item1->row(), 0);

  if(item2)
    {
      QString title(item.title().trimmed());

      if(title.isEmpty())
	title = item.url().toString();

      if(title.isEmpty())
	title = tr("Dooble");

      item2->setText(title);
    }

  QTableWidgetItem *item3 = m_ui.table->item(item1->row(), 2);

  if(item3)
    {
      item3->setText(item.lastVisited().toString(Qt::ISODate));
      set_row_hidden(item3->row());
    }
}

void dooble_history_window::slot_new_item(const QIcon &icon,
					  const QWebEngineHistoryItem &item)
{
  if(!item.isValid())
    return;

  QString title(item.title().trimmed());

  if(title.isEmpty())
    title = item.url().toString();

  if(title.isEmpty())
    title = tr("Dooble");

  QTableWidgetItem *item1 = 0;

  if(icon.isNull())
    item1 = new QTableWidgetItem(dooble_favicons::icon(item.url()), title);
  else
    item1 = new QTableWidgetItem(icon, title);

  item1->setData(Qt::UserRole, item.url());
  item1->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item1->setToolTip(item1->text());
  m_items[item.url()] = item1;

  QTableWidgetItem *item2 = new QTableWidgetItem(item.url().toString());

  item2->setData(Qt::UserRole, item.url());
  item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  item2->setToolTip(item2->text());

  QTableWidgetItem *item3 = new QTableWidgetItem
    (item.lastVisited().toString(Qt::ISODate));

  item3->setData(Qt::UserRole, item.url());
  item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  m_ui.table->setSortingEnabled(false);
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 0, item1);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 1, item2);
  m_ui.table->setItem(m_ui.table->rowCount() - 1, 2, item3);

  /*
  ** Hide or show the new row.
  */

  set_row_hidden(m_ui.table->rowCount() - 1);
  m_ui.table->setSortingEnabled(true);
}

void dooble_history_window::slot_parent_destroyed(void)
{
  m_parent = 0;
}

void dooble_history_window::slot_populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;
  else if(!dooble::s_history)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_items.clear();
  m_ui.table->setSortingEnabled(false);

  QHash<QUrl, QHash<int, QVariant> > hash(dooble::s_history->history());
  QHashIterator<QUrl, QHash<int, QVariant> > it(hash);
  int i = 0;

  m_ui.table->setRowCount(hash.size());

  while(it.hasNext())
    {
      it.next();

      QDateTime last_visited
	(it.value().value(dooble_history::LAST_VISITED).toDateTime());
      QIcon icon(it.value().value(dooble_history::FAVICON).value<QIcon> ());
      QString title(it.value().value(dooble_history::TITLE).toString());
      QTableWidgetItem *item1 = 0;
      QTableWidgetItem *item2 = 0;
      QTableWidgetItem *item3 = 0;
      QUrl url(it.value().value(dooble_history::URL).toUrl());

      item1 = new QTableWidgetItem(icon, title);
      item1->setData(Qt::UserRole, url);
      item1->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item1->setToolTip(item1->text());
      item2 = new QTableWidgetItem(url.toString());
      item2->setData(Qt::UserRole, url);
      item2->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      item2->setToolTip(item2->text());
      item3 = new QTableWidgetItem(last_visited.toString(Qt::ISODate));
      item3->setData(Qt::UserRole, url);
      item3->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_items[url] = item1;
      m_ui.table->setItem(i, 0, item1);
      m_ui.table->setItem(i, 1, item2);
      m_ui.table->setItem(i, 2, item3);
      i += 1;
    }

  m_ui.table->setSortingEnabled(true);
  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_search_timer_timeout(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QDateTime period(QDateTime::currentDateTime());
  QString text(m_ui.search->text().toLower().trimmed());

  switch(m_ui.period->currentRow())
    {
    case 0: // All
      {
	break;
      }
    case 1: // Today
      {
	break;
      }
    case 2: // Yesterday
      {
	period = period.addDays(-1);
	break;
      }
    case 3: // This Month
      {
	break;
      }
    case 4: // Previous Month
      {
	period = period.addMonths(-1);
	break;
      }
    default:
      {
	break;
      }
    }

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(m_ui.period->currentRow() <= 0 && text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	QTableWidgetItem *item1 = m_ui.table->item(i, 0);
	QTableWidgetItem *item2 = m_ui.table->item(i, 1);
	QTableWidgetItem *item3 = m_ui.table->item(i, 2);

	if(!item1 || !item2 || !item3)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	switch(m_ui.period->currentRow())
	  {
	  case 0: // All
	    {
	      if(text.isEmpty())
		m_ui.table->setRowHidden(i, false);
	      else if(item1->text().toLower().contains(text) ||
		      item2->text().toLower().contains(text))
		m_ui.table->setRowHidden(i, false);
	      else
		m_ui.table->setRowHidden(i, true);

	      break;
	    }
	  case 1: // Today
	  case 2: // Yesterday
	    {
	      QDateTime dateTime
		(QDateTime::fromString(item3->text(), Qt::ISODate));

	      if(dateTime.date() == period.date())
		{
		  if(text.isEmpty())
		    m_ui.table->setRowHidden(i, false);
		  else if(item1->text().toLower().contains(text) ||
			  item2->text().toLower().contains(text))
		    m_ui.table->setRowHidden(i, false);
		  else
		    m_ui.table->setRowHidden(i, true);
		}
	      else
		m_ui.table->setRowHidden(i, true);

	      break;
	    }
	  default:
	    {
	      QDateTime dateTime
		(QDateTime::fromString(item3->text(), Qt::ISODate));

	      if(dateTime.date().month() == period.date().month() &&
		 dateTime.date().year() == period.date().year())
		{
		  if(text.isEmpty())
		    m_ui.table->setRowHidden(i, false);
		  else if(item1->text().toLower().contains(text) ||
			  item2->text().toLower().contains(text))
		    m_ui.table->setRowHidden(i, false);
		  else
		    m_ui.table->setRowHidden(i, true);
		}
	      else
		m_ui.table->setRowHidden(i, true);

	      break;
	    }
	  }
      }

  QApplication::restoreOverrideCursor();
}

void dooble_history_window::slot_show_context_menu(const QPoint &point)
{
  QMenu menu(this);

  menu.addAction(tr("&Copy Location"), this, SLOT(slot_copy_location(void)));
  menu.addSeparator();
  menu.addAction(tr("&Delete Page(s)"), this, SLOT(slot_delete_pages(void)));
  menu.exec(mapToGlobal(point));
}
