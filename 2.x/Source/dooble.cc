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

#include <QKeyEvent>
#include <QMenu>
#include <QUrl>
#include <QWebEngineProfile>

#include "dooble.h"
#include "dooble_accepted_or_blocked_domains.h"
#include "dooble_application.h"
#include "dooble_clear_items.h"
#include "dooble_cookies.h"
#include "dooble_cookies_window.h"
#include "dooble_cryptography.h"
#include "dooble_favicons.h"
#include "dooble_history.h"
#include "dooble_history_window.h"
#include "dooble_page.h"
#include "dooble_ui_utilities.h"
#include "dooble_web_engine_url_request_interceptor.h"
#include "dooble_web_engine_view.h"

QPointer<dooble_history> dooble::s_history;
bool dooble::s_containers_populated = false;
dooble_accepted_or_blocked_domains *dooble::s_accepted_or_blocked_domains = 0;
dooble_application *dooble::s_application = 0;
dooble_clear_items *dooble::s_clear_items = 0;
dooble_cookies *dooble::s_cookies = 0;
dooble_cookies_window *dooble::s_cookies_window = 0;
dooble_cryptography *dooble::s_cryptography = 0;
dooble_history_window *dooble::s_history_window = 0;
dooble_settings *dooble::s_settings = 0;
dooble_web_engine_url_request_interceptor *dooble::s_url_request_interceptor =
  0;

dooble::dooble(dooble_page *page):QMainWindow()
{
  initialize_static_members();
  m_ui.setupUi(this);
  connect_signals();
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#else
  setMenuBar(0);
#endif
  new_page(page);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(2500);
	s_containers_populated = true;
      }
}

dooble::dooble(dooble_web_engine_view *view):QMainWindow()
{
  initialize_static_members();
  m_ui.setupUi(this);
  connect_signals();
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#else
  setMenuBar(0);
#endif
  new_page(view);

  if(!s_containers_populated)
    if(s_cryptography->as_plaintext())
      {
	m_populate_containers_timer.setSingleShot(true);
	m_populate_containers_timer.start(2500);
	s_containers_populated = true;
      }
}

dooble::dooble(void):dooble(static_cast<dooble_web_engine_view *> (0))
{
}

void dooble::closeEvent(QCloseEvent *event)
{
  if(dooble_settings::setting("save_geometry").toBool())
    dooble_settings::set_setting("dooble_geometry", saveGeometry().toBase64());

  QWidgetList list(QApplication::topLevelWidgets());

  for(int i = 0; i < list.size(); i++)
    if(list.at(i) != this && qobject_cast<dooble *> (list.at(i)))
      {
	deleteLater();
	return;
      }

  if(s_cookies_window)
    s_cookies_window->close();

  if(s_history)
    s_history->deleteLater();

  QMainWindow::closeEvent(event);
  QApplication::exit(0);
}

void dooble::connect_signals(void)
{
  connect(&m_populate_containers_timer,
	  SIGNAL(timeout(void)),
	  this,
	  SLOT(slot_populate_containers_timer_timeout(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_edit,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_edit,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_file,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_file,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_tools,
	  SIGNAL(aboutToHide(void)),
	  this,
	  SLOT(slot_about_to_hide_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.menu_tools,
	  SIGNAL(aboutToShow(void)),
	  this,
	  SLOT(slot_about_to_show_main_menu(void)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(currentChanged(int)),
	  this,
	  SLOT(slot_tab_index_changed(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(new_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SLOT(slot_open_tab_as_new_window(int)),
	  Qt::UniqueConnection);
  connect(m_ui.tab,
	  SIGNAL(tabCloseRequested(int)),
	  this,
	  SLOT(slot_tab_close_requested(int)),
	  Qt::UniqueConnection);
  connect(s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)),
	  Qt::UniqueConnection);
  connect(this,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  Qt::UniqueConnection);
}

void dooble::initialize_static_members(void)
{
  if(!s_accepted_or_blocked_domains)
    s_accepted_or_blocked_domains = new dooble_accepted_or_blocked_domains();

  if(!s_clear_items)
    s_clear_items = new dooble_clear_items();

  if(!s_cookies)
    s_cookies = new dooble_cookies(false, 0);

  if(!s_cookies_window)
    {
      s_cookies_window = new dooble_cookies_window(false, 0);
      s_cookies_window->setCookieStore
	(QWebEngineProfile::defaultProfile()->cookieStore());
      s_cookies_window->setCookies(s_cookies);
    }

  if(!s_cryptography)
    {
      if(dooble_settings::setting("credentials_enabled").toBool())
	s_cryptography = new dooble_cryptography();
      else
	s_cryptography = new dooble_cryptography(QByteArray(), QByteArray());
    }

  if(!s_history)
    s_history = new dooble_history();

  if(!s_history_window)
    s_history_window = new dooble_history_window();

  if(!s_url_request_interceptor)
    {
      s_url_request_interceptor = new
	dooble_web_engine_url_request_interceptor(0);
      QWebEngineProfile::defaultProfile()->setRequestInterceptor
	(s_url_request_interceptor);
    }
}

void dooble::keyPressEvent(QKeyEvent *event)
{
  if(event && event->modifiers() == Qt::AltModifier)
    {
      dooble_page *page = qobject_cast<dooble_page *>
	(m_ui.tab->currentWidget());

      if(page && page->menu())
	{
	  page->menu()->showMenu();
	  page->menu()->menu()->activateWindow();
	}
    }

  QMainWindow::keyPressEvent(event);
}

void dooble::new_page(bool is_private)
{
  dooble_page *page = new dooble_page(is_private, 0, m_ui.tab);

  prepare_page_connections(page);
  m_ui.tab->addTab(page, tr("Dooble"));

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page);

  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  page->address_widget()->setFocus();
}

void dooble::new_page(dooble_page *page)
{
  if(!page)
    return;

  page->setParent(m_ui.tab);
  prepare_page_connections(page);

  /*
  ** The page's icon and title may not be meaningful.
  */

  QString title(page->title().trimmed());

  if(title.isEmpty())
    title = page->url().toString();

  if(title.isEmpty())
    title = tr("Dooble");

  m_ui.tab->addTab(page, title);

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page);

  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  page->address_widget()->setFocus();
}

void dooble::new_page(dooble_web_engine_view *view)
{
  dooble_page *page = new dooble_page(false, view, m_ui.tab);

  prepare_page_connections(page);
  m_ui.tab->addTab(page, tr("Dooble"));

  if(dooble_settings::setting("access_new_tabs").toBool())
    m_ui.tab->setCurrentWidget(page);

  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), page->icon()); // Mac too!
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  page->address_widget()->setFocus();
}

void dooble::prepare_page_connections(dooble_page *page)
{
  if(!page)
    return;

  connect(dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  page,
	  SLOT(slot_dooble_credentials_authenticated(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(close_tab(void)),
	  this,
	  SLOT(slot_close_tab(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(create_tab(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_tab(dooble_web_engine_view *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(create_window(dooble_web_engine_view *)),
	  this,
	  SLOT(slot_create_window(dooble_web_engine_view *)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  dooble::s_application,
	  SIGNAL(dooble_credentials_authenticated(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(iconChanged(const QIcon &)),
	  this,
	  SLOT(slot_icon_changed(const QIcon &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadFinished(bool)),
	  m_ui.tab,
	  SLOT(slot_load_finished(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadFinished(bool)),
	  this,
	  SLOT(slot_load_finished(bool)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadStarted(void)),
	  m_ui.tab,
	  SLOT(slot_load_started(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(loadStarted(void)),
	  this,
	  SLOT(slot_load_started(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_private_tab(void)),
	  this,
	  SLOT(slot_new_private_tab(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_tab(void)),
	  this,
	  SLOT(slot_new_tab(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(new_window(void)),
	  this,
	  SLOT(slot_new_window(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(quit_dooble(void)),
	  this,
	  SLOT(slot_quit_dooble(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_blocked_domains(void)),
	  this,
	  SLOT(slot_show_blocked_domains(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_clear_items(void)),
	  this,
	  SLOT(slot_show_clear_items(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_history(void)),
	  this,
	  SLOT(slot_show_history(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_settings(void)),
	  this,
	  SLOT(slot_show_settings(void)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(show_settings_panel(dooble_settings::Panels)),
	  this,
	  SLOT(slot_show_settings_panel(dooble_settings::Panels)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
  connect(page,
	  SIGNAL(titleChanged(const QString &)),
	  this,
	  SLOT(slot_title_changed(const QString &)),
	  static_cast<Qt::ConnectionType> (Qt::AutoConnection |
					   Qt::UniqueConnection));
}

void dooble::show(void)
{
  if(dooble_settings::setting("save_geometry").toBool())
    restoreGeometry(QByteArray::fromBase64(dooble_settings::
					   setting("dooble_geometry").
					   toByteArray()));

  QMainWindow::show();
}

void dooble::slot_about_to_hide_main_menu(void)
{
  QMenu *menu = qobject_cast<QMenu *> (sender());

  if(menu)
    menu->clear();
}

void dooble::slot_about_to_show_main_menu(void)
{
  QMenu *menu = qobject_cast<QMenu *> (sender());

  if(menu)
    {
      menu->clear();

      dooble_page *page = qobject_cast<dooble_page *>
	(m_ui.tab->currentWidget());

      if(page &&
	 page->menu() &&
	 page->menu()->menu() &&
	 page->menu()->menu()->actions().size() >= 3)
	{
	  if(m_ui.menu_edit == menu &&
	     page->menu()->menu()->actions()[1]->menu())
	    m_ui.menu_edit->addActions
	      (page->menu()->menu()->actions()[1]->menu()->actions());
	  else if(m_ui.menu_file == menu &&
		  page->menu()->menu()->actions()[1]->menu())
	    {
	      m_ui.menu_file->addActions
		(page->menu()->menu()->actions()[0]->menu()->actions());

	      if(page->action_close_tab())
		page->action_close_tab()->setEnabled(m_ui.tab->count() > 1);
	    }
	  else if(m_ui.menu_tools == menu &&
		  page->menu()->menu()->actions()[2]->menu())
	    m_ui.menu_tools->addActions
	      (page->menu()->menu()->actions()[2]->menu()->actions());
	}
    }
}

void dooble::slot_close_tab(void)
{
  if(m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  m_ui.tab->removeTab(m_ui.tab->indexOf(page));
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
  page->deleteLater();
}

void dooble::slot_create_tab(dooble_web_engine_view *view)
{
  new_page(view);
}

void dooble::slot_create_window(dooble_web_engine_view *view)
{
  dooble *d = new dooble(view);

  d->show();
}

void dooble::slot_icon_changed(const QIcon &icon)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  if(dooble::s_history)
    dooble::s_history->save_favicon(icon, page->url());

  dooble_favicons::save_favicon(icon, page->url());
  m_ui.tab->setTabIcon(m_ui.tab->indexOf(page), icon);
}

void dooble::slot_load_finished(bool ok)
{
  Q_UNUSED(ok);

  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  dooble_favicons::save_favicon(page->icon(), page->url());
}

void dooble::slot_load_started(void)
{
}

void dooble::slot_new_private_tab(void)
{
  new_page(true);
}

void dooble::slot_new_tab(void)
{
  new_page(static_cast<dooble_web_engine_view *> (0));
}

void dooble::slot_new_window(void)
{
  dooble *d = new dooble();

  d->show();
}

void dooble::slot_open_tab_as_new_window(int index)
{
  if(m_ui.tab->count() == 1)
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;

  dooble *d = new dooble(page);

  d->show();
  m_ui.tab->removeTab(m_ui.tab->indexOf(page));
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
}

void dooble::slot_open_url(const QUrl &url)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->currentWidget());

  if(page)
    page->load(url);
}

void dooble::slot_populate_containers_timer_timeout(void)
{
  emit dooble_credentials_authenticated(true);
}

void dooble::slot_quit_dooble(void)
{
  /*
  ** May require some confirmation from the user.
  */

  close();
  QApplication::exit(0);
}

void dooble::slot_settings_applied(void)
{
#ifndef Q_OS_MACOS
  m_ui.menu_bar->setVisible
    (dooble_settings::setting("main_menu_bar_visible").toBool());
#endif
}

void dooble::slot_show_blocked_domains(void)
{
  s_accepted_or_blocked_domains->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget
      (this, s_accepted_or_blocked_domains);

  s_accepted_or_blocked_domains->activateWindow();
  s_accepted_or_blocked_domains->raise();
}

void dooble::slot_show_clear_items(void)
{
  s_clear_items->exec();
}

void dooble::slot_show_history(void)
{
  s_history_window->showNormal(this);

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, s_history_window);

  s_history_window->activateWindow();
  s_history_window->raise();
}

void dooble::slot_show_settings(void)
{
  s_settings->showNormal();

  if(dooble_settings::setting("center_child_windows").toBool())
    dooble_ui_utilities::center_window_widget(this, s_settings);

  s_settings->activateWindow();
  s_settings->raise();
}

void dooble::slot_show_settings_panel(dooble_settings::Panels panel)
{
  slot_show_settings();
  s_settings->show_panel(panel);
}

void dooble::slot_tab_close_requested(int index)
{
  if(index < 0 || m_ui.tab->count() < 2) // Safety.
    return;

  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;

  page->deleteLater();
  m_ui.tab->removeTab(index);
  m_ui.tab->setTabsClosable(m_ui.tab->count() > 1);
}

void dooble::slot_tab_index_changed(int index)
{
  dooble_page *page = qobject_cast<dooble_page *> (m_ui.tab->widget(index));

  if(!page)
    return;
  else if(page != m_ui.tab->currentWidget())
    return;

  if(page->title().trimmed().isEmpty())
    setWindowTitle(tr("Dooble"));
  else
    setWindowTitle(tr("%1 - Dooble").arg(page->title().trimmed()));

  page->view()->setFocus();
}

void dooble::slot_title_changed(const QString &title)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;
  else if(page != m_ui.tab->currentWidget())
    return;

  QString text(title.trimmed());

  if(text.isEmpty())
    text = page->url().toString();

  if(text.isEmpty())
    {
      text = tr("Dooble");
      setWindowTitle(text);
    }
  else
    setWindowTitle(tr("%1 - Dooble").arg(text));

  m_ui.tab->setTabText(m_ui.tab->indexOf(page), text);
  m_ui.tab->setTabToolTip(m_ui.tab->indexOf(page), text);
}
