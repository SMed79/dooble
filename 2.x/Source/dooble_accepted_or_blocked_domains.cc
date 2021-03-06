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

#include <QDir>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMessageBox>
#include <QShortcut>
#include <QSqlQuery>
#include <QUrl>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_cryptography.h"
#include "dooble_settings.h"

dooble_accepted_or_blocked_domains::dooble_accepted_or_blocked_domains(void):
  QMainWindow()
{
  m_search_timer.setInterval(750);
  m_search_timer.setSingleShot(true);
  m_ui.setupUi(this);
  m_ui.table->sortByColumn(1, Qt::AscendingOrder);
  connect(&m_search_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_search_timer_timeout(void)));
  connect(m_ui.accept_mode,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slot_radio_button_toggled(bool)));
  connect(m_ui.action_import,
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slot_import(void)));
  connect(m_ui.add,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_add(void)));
  connect(m_ui.block_mode,
	  SIGNAL(clicked(bool)),
	  this,
	  SLOT(slot_radio_button_toggled(bool)));
  connect(m_ui.delete_rows,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slot_delete_rows(void)));
  connect(m_ui.search,
	  SIGNAL(textEdited(const QString &)),
	  &m_search_timer,
	  SLOT(start(void)));

  if(dooble_settings::
     setting("accepted_or_blocked_domains_mode").toString() == "accept")
    m_ui.accept_mode->click();
  else
    m_ui.block_mode->click();

  new QShortcut
    (QKeySequence(tr("Ctrl+F")), m_ui.search, SLOT(setFocus(void)));
}

bool dooble_accepted_or_blocked_domains::contains(const QString &domain) const
{
  return m_domains.value(domain.toLower().trimmed(), 0) == 1;
}

void dooble_accepted_or_blocked_domains::accept_or_block_domain
(const QString &domain)
{
  if(domain.trimmed().isEmpty())
    return;
  else if(m_domains.contains(domain.toLower().trimmed()))
    return;

  m_domains[domain.toLower().trimmed()] = 1;
  m_ui.table->setRowCount(m_ui.table->rowCount() + 1);
  m_ui.table->setSortingEnabled(false);
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  for(int i = 0; i < 2; i++)
    {
      QTableWidgetItem *item = new QTableWidgetItem();

      if(i == 0)
	{
	  item->setFlags(Qt::ItemIsEnabled |
			 Qt::ItemIsSelectable |
			 Qt::ItemIsUserCheckable);
	  item->setCheckState(Qt::Checked);
	}
      else
	{
	  item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	  item->setText(domain.toLower().trimmed());
	}

      m_ui.table->setItem(m_ui.table->rowCount() - 1, i, item);
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  save_blocked_domain(domain.toLower().trimmed(), true);
}

void dooble_accepted_or_blocked_domains::closeEvent(QCloseEvent *event)
{
  QMainWindow::closeEvent(event);
}

void dooble_accepted_or_blocked_domains::keyPressEvent(QKeyEvent *event)
{
  if(event && event->key() == Qt::Key_Escape)
    close();

  QMainWindow::keyPressEvent(event);
}

void dooble_accepted_or_blocked_domains::populate(void)
{
  if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_domains.clear();
  m_ui.table->clearContents();

  QMap<QString, int> oids;
  QString database_name("dooble_accepted_or_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.setForwardOnly(true);

	if(query.exec("SELECT domain, state, OID "
		      "FROM dooble_accepted_or_blocked_domains"))
	  while(query.next())
	    {
	      QByteArray data1
		(QByteArray::fromBase64(query.value(0).toByteArray()));

	      data1 = dooble::s_cryptography->mac_then_decrypt(data1);

	      if(data1.isEmpty())
		continue;

	      QByteArray data2
		(QByteArray::fromBase64(query.value(1).toByteArray()));

	      data2 = dooble::s_cryptography->mac_then_decrypt(data2);

	      if(data2.isEmpty())
		continue;

	      m_domains[data1.constData()] = QVariant(data2).toBool();
	      oids[data1.constData()] = query.value(2).toInt();
	    }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  m_ui.table->setRowCount(m_domains.size());
  m_ui.table->setSortingEnabled(false);
  disconnect(m_ui.table,
	     SIGNAL(itemChanged(QTableWidgetItem *)),
	     this,
	     SLOT(slot_item_changed(QTableWidgetItem *)));

  QHashIterator<QString, char> it(m_domains);
  int i = 0;

  while(it.hasNext())
    {
      it.next();

      QTableWidgetItem *item = new QTableWidgetItem();

      if(it.value())
	item->setCheckState(Qt::Checked);
      else
	item->setCheckState(Qt::Unchecked);

      item->setData(Qt::UserRole, oids.value(it.key()));
      item->setFlags(Qt::ItemIsEnabled |
		     Qt::ItemIsSelectable |
		     Qt::ItemIsUserCheckable);
      m_ui.table->setItem(i, 0, item);
      item = new QTableWidgetItem(it.key());
      item->setData(Qt::UserRole, oids.value(it.key()));
      item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
      m_ui.table->setItem(i, 1, item);
      i += 1;
    }

  connect(m_ui.table,
	  SIGNAL(itemChanged(QTableWidgetItem *)),
	  this,
	  SLOT(slot_item_changed(QTableWidgetItem *)));
  m_ui.table->setSortingEnabled(true);
  m_ui.table->sortByColumn
    (1, m_ui.table->horizontalHeader()->sortIndicatorOrder());
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::purge(void)
{
  QString database_name("dooble_accepted_or_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");
	query.exec("DELETE FROM dooble_accepted_or_blocked_domains");
	query.exec("VACUUM");
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
}

void dooble_accepted_or_blocked_domains::resizeEvent(QResizeEvent *event)
{
  QMainWindow::resizeEvent(event);
  save_settings();
}

void dooble_accepted_or_blocked_domains::save_blocked_domain
(const QString &domain, bool state)
{
  if(domain.trimmed().isEmpty())
    return;
  else if(!dooble::s_cryptography || !dooble::s_cryptography->authenticated())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_accepted_or_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.exec
	  ("CREATE TABLE IF NOT EXISTS dooble_accepted_or_blocked_domains ("
	   "domain TEXT NOT NULL, "
	   "domain_digest TEXT NOT NULL PRIMARY KEY, "
	   "state TEXT NOT NULL)");
	query.exec("PRAGMA synchronous = OFF");
	query.prepare
	  ("INSERT OR REPLACE INTO dooble_accepted_or_blocked_domains "
	   "(domain, domain_digest, state) VALUES (?, ?, ?)");

	QByteArray data
	  (dooble::s_cryptography->
	   encrypt_then_mac(domain.toLower().trimmed().toUtf8()));

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->hmac(domain.toLower().trimmed());
	ok &= !data.isEmpty();

	if(ok)
	  query.addBindValue(data.toBase64());

	data = dooble::s_cryptography->encrypt_then_mac
	  (state ? "true" : "false");

	if(data.isEmpty())
	  ok = false;
	else
	  query.addBindValue(data.toBase64());

	if(ok)
	  query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::save_settings(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting
      ("accepted_or_blocked_domains_geometry", saveGeometry().toBase64());
}

void dooble_accepted_or_blocked_domains::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("accepted_or_blocked_domains_geometry").
			      toByteArray()));

  QMainWindow::show();
  populate();
}

void dooble_accepted_or_blocked_domains::showNormal(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry
      (QByteArray::fromBase64(dooble_settings::
			      setting("accepted_or_blocked_domains_geometry").
			      toByteArray()));

  QMainWindow::showNormal();
  populate();
}

void dooble_accepted_or_blocked_domains::slot_add(void)
{
  QInputDialog dialog(this);

  dialog.setLabelText(tr("Domain / URL"));
  dialog.setTextEchoMode(QLineEdit::Normal);
  dialog.setWindowIcon(windowIcon());
  dialog.setWindowTitle(tr("Dooble: New Domain"));

  if(dialog.exec() != QDialog::Accepted)
    return;

  QString text = QUrl::fromUserInput
    (dialog.textValue().toLower().trimmed()).host();

  if(text.isEmpty())
    return;

  accept_or_block_domain(text);
}

void dooble_accepted_or_blocked_domains::slot_delete_rows(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QModelIndexList list(m_ui.table->selectionModel()->selectedRows(1));

  QApplication::restoreOverrideCursor();

  if(list.size() > 0)
    {
      QMessageBox mb(this);

      mb.setIcon(QMessageBox::Question);
      mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
      mb.setText
	(tr("Are you sure that you wish to delete the selected domain(s)?"));
      mb.setWindowIcon(windowIcon());
      mb.setWindowModality(Qt::WindowModal);
      mb.setWindowTitle(tr("Dooble: Confirmation"));

      if(mb.exec() != QMessageBox::Yes)
	return;
    }

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString database_name("dooble_accepted_or_blocked_domains");

  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", database_name);

    db.setDatabaseName(dooble_settings::setting("home_path").toString() +
		       QDir::separator() +
		       "dooble_accepted_or_blocked_domains.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.exec("PRAGMA synchronous = OFF");

	for(int i = list.size() - 1; i >= 0; i--)
	  {
	    query.prepare
	      ("DELETE FROM dooble_accepted_or_blocked_domains WHERE OID = ?");
	    query.addBindValue(list.at(i).data(Qt::UserRole));

	    if(query.exec())
	      {
		m_domains.remove(list.at(i).data().toString());
		m_ui.table->removeRow(list.at(i).row());
	      }
	  }
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(database_name);
  QApplication::restoreOverrideCursor();
}

void dooble_accepted_or_blocked_domains::slot_import(void)
{
  QFileDialog dialog(this);

  dialog.setAcceptMode(QFileDialog::AcceptOpen);
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::ExistingFile);
  dialog.setLabelText(QFileDialog::Accept, tr("Select"));
  dialog.setNameFilter(tr("dooble_blocked_domains.txt"));
  dialog.setWindowTitle(tr("Dooble: Import Blocked Domains"));

  if(dialog.exec() == QDialog::Accepted)
    {
      QFile file(dialog.selectedFiles().value(0));

      if(file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  repaint();
	  QApplication::processEvents();

	  QProgressDialog progress(this);

	  progress.setMaximum(0);
	  progress.setMinimum(0);
	  progress.setWindowModality(Qt::ApplicationModal);
	  progress.setWindowTitle(tr("Dooble: Progress"));
	  progress.show();
	  progress.update();

	  QByteArray data(2048, 0);
	  qint64 line = 0;
	  qint64 rc = 0;

	  while((rc = file.readLine(data.data(),
				    static_cast<qint64> (data.length()))) >= 0)
	    {
	      if(progress.wasCanceled())
		break;

	      QUrl url(QUrl::fromUserInput(data.mid(0, static_cast<int> (rc)).
					   trimmed()));

	      if(!url.isEmpty() && url.isValid())
		save_blocked_domain(url.host(), true);

	      line += 1;
	      progress.setLabelText(tr("Line %1 processed.").arg(line));
	      progress.update();
	      QApplication::processEvents();
	    }

	  file.close();
	  populate();
	}
    }
}

void dooble_accepted_or_blocked_domains::slot_item_changed
(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 0)
    return;

  bool state = item->checkState() == Qt::Checked;

  item = m_ui.table->item(item->row(), 1);

  if(!item)
    return;

  m_domains[item->text()] = state;
  save_blocked_domain(item->text(), state);
}

void dooble_accepted_or_blocked_domains::slot_populate(void)
{
  populate();
}

void dooble_accepted_or_blocked_domains::slot_radio_button_toggled(bool state)
{
  if(m_ui.accept_mode == sender() && state)
    {
      dooble_settings::set_setting
	("accepted_or_blocked_domains_mode", "accept");
      m_ui.table->setHorizontalHeaderLabels
	(QStringList() << tr("Accepted") << tr("Domain"));
    }
  else if(m_ui.block_mode == sender())
    {
      dooble_settings::set_setting
	("accepted_or_blocked_domains_mode", "block");
      m_ui.table->setHorizontalHeaderLabels
	(QStringList() << tr("Blocked") << tr("Domain"));
    }
}

void dooble_accepted_or_blocked_domains::slot_search_timer_timeout(void)
{
  QString text(m_ui.search->text().toLower().trimmed());

  for(int i = 0; i < m_ui.table->rowCount(); i++)
    if(text.isEmpty())
      m_ui.table->setRowHidden(i, false);
    else
      {
	QTableWidgetItem *item = m_ui.table->item(i, 1);

	if(!item)
	  {
	    m_ui.table->setRowHidden(i, false);
	    continue;
	  }

	if(item->text().contains(text))
	  m_ui.table->setRowHidden(i, false);
	else
	  m_ui.table->setRowHidden(i, true);
      }
}
