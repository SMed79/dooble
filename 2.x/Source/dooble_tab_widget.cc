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

#include <QLabel>
#include <QMovie>

#include "dooble.h"
#include "dooble_application.h"
#include "dooble_page.h"
#include "dooble_tab_bar.h"
#include "dooble_tab_widget.h"

dooble_tab_widget::dooble_tab_widget(QWidget *parent):QTabWidget(parent)
{
  m_tab_bar = new dooble_tab_bar(this);
  connect(m_tab_bar,
	  SIGNAL(new_tab(void)),
	  this,
	  SIGNAL(new_tab(void)));
  connect(m_tab_bar,
	  SIGNAL(open_tab_as_new_window(int)),
	  this,
	  SIGNAL(open_tab_as_new_window(int)));
  setTabBar(m_tab_bar);
}

dooble_page *dooble_tab_widget::page(int index) const
{
  return qobject_cast<dooble_page *> (widget(index));
}

void dooble_tab_widget::setTabIcon(int index, const QIcon &icon)
{
#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    QTabWidget::setTabIcon(index, icon);
  else
    {
      QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->
	styleHint(QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);

      side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide :
	QTabBar::RightSide;

      QLabel *label = qobject_cast<QLabel *>
	(m_tab_bar->tabButton(index, side));

      if(!label)
	{
	  label = new QLabel(this);
	  label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	  m_tab_bar->setTabButton(index, side, 0);
	  m_tab_bar->setTabButton(index, side, label);
	}
      else if(!label->movie() || label->movie()->state() != QMovie::Running)
	label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));

      label->setProperty("icon", icon);
    }
#else
  QTabWidget::setTabIcon(index, icon);
#endif
}

void dooble_tab_widget::slot_load_finished(void)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);
  int index = indexOf(page);

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  QLabel *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(label)
    {
      QMovie *movie = label->movie();

      if(movie)
	{
	  movie->stop();
	  movie->deleteLater();
	}

      label->setMovie(0);

#ifdef Q_OS_MACOS
      if(dooble::s_application->style_name() == "fusion")
	{
	  QIcon icon(label->property("icon").value<QIcon> ());

	  label->setPixmap(icon.pixmap(icon.actualSize(QSize(16, 16))));
	}
#endif

      m_tab_bar->repaint();
    }
}

void dooble_tab_widget::slot_load_started(void)
{
  dooble_page *page = qobject_cast<dooble_page *> (sender());

  if(!page)
    return;

  QTabBar::ButtonPosition side = (QTabBar::ButtonPosition) style()->styleHint
    (QStyle::SH_TabBar_CloseButtonPosition, 0, m_tab_bar);
  int index = indexOf(page);

#ifdef Q_OS_MACOS
  if(dooble::s_application->style_name() == "fusion")
    side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
  else
    side = (side == QTabBar::LeftSide) ? QTabBar::LeftSide : QTabBar::RightSide;
#else
  side = (side == QTabBar::LeftSide) ? QTabBar::RightSide : QTabBar::LeftSide;
#endif

  QLabel *label = qobject_cast<QLabel *> (m_tab_bar->tabButton(index, side));

  if(!label)
    {
      QPixmap pixmap(16, 16);

      pixmap.fill(m_tab_bar->backgroundRole());
      label = new QLabel(this);
      label->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
      label->setPixmap(pixmap);
      m_tab_bar->setTabButton(index, side, 0);
      m_tab_bar->setTabButton(index, side, label);
    }

  QMovie *movie = label->movie();

  if(!movie)
    {
      movie = new QMovie(":/spinning_wheel.gif", QByteArray(), label);
      label->setMovie(movie);
      movie->setScaledSize(QSize(16, 16));
      movie->start();
    }
  else
    {
      if(movie->state() != QMovie::Running)
	movie->start();
    }
}
