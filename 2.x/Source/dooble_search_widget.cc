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

#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>

#include "dooble.h"
#include "dooble_search_widget.h"
#include "dooble_settings.h"

dooble_search_widget::dooble_search_widget(QWidget *parent):QLineEdit(parent)
{
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  m_find_tool_button = new QToolButton(this);
  m_find_tool_button->setCursor(Qt::ArrowCursor);
  m_find_tool_button->setIconSize(QSize(16, 16));
  m_find_tool_button->setStyleSheet("QToolButton {"
				    "border: none; "
				    "padding-bottom: 0px;}");
  connect(dooble::s_settings,
	  SIGNAL(applied(void)),
	  this,
	  SLOT(slot_settings_applied(void)));
  prepare_icons();
  setMinimumHeight(sizeHint().height());
  setStyleSheet
    (QString("QLineEdit {padding-left: %1px; padding-right: %2px;}").
     arg(frame_width + m_find_tool_button->sizeHint().width() + 5).
     arg(frame_width + 5));
}

void dooble_search_widget::keyPressEvent(QKeyEvent *event)
{
  QLineEdit::keyPressEvent(event);
}

void dooble_search_widget::prepare_icons(void)
{
  QString icon_set(dooble_settings::setting("icon_set").toString());

  m_find_tool_button->setIcon(QIcon(QString(":/%1/16/find.png").arg(icon_set)));
}

void dooble_search_widget::resizeEvent(QResizeEvent *event)
{
  QSize size1 = m_find_tool_button->sizeHint();
  int d = 0;
  int frame_width = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  d = (rect().height() - size1.height()) / 2;
  m_find_tool_button->move(frame_width - rect().left() + 5, rect().top() + d);
  QLineEdit::resizeEvent(event);
}

void dooble_search_widget::slot_settings_applied(void)
{
  prepare_icons();
}
