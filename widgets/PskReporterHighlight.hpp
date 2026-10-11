#ifndef PSK_REPORTER_HIGHLIGHT_HPP
#define PSK_REPORTER_HIGHLIGHT_HPP

#include "PskReporterDialog.hpp"
#include <QHelpEvent>
#include <QTextBlock>
#include <QTextEdit>
#include <QToolTip>

// Visual overlays preserve the decoded text and existing QSO/DXCC formatting.
class PskReporterHighlight final : public QObject
{
public:
  using HeardStations = QMap<QString, qint64>;

  PskReporterHighlight (QTextEdit *display, PskReporterDialog::ContextProvider context)
    : QObject {display}, display_ {display}, context_ {context}
  {
    display_->viewport ()->installEventFilter (this);
    auto timer = new QTimer {this};
    timer->setInterval (1000);
    connect (timer, &QTimer::timeout, this, [this] { update (); });
    timer->start ();
    connect (display_->document (), &QTextDocument::contentsChanged, this, [this] {
      if (pending_) return;
      pending_ = true;
      QTimer::singleShot (0, this, [this] { pending_ = false; update (true); });
    });
  }

  void setReports (HeardStations const& heard, PskReporterDialog::Context const& context)
  {
    heard_ = heard;
    loaded_ = context;
    update (true);
  }

private:
  enum { MarkerProperty = QTextFormat::UserProperty + 731 };
  void update (bool force = false)
  {
    auto current = context_ ();
    QSet<QString> active;
    auto now = QDateTime::currentSecsSinceEpoch ();
    if (current.call == loaded_.call && current.mode == loaded_.mode && current.dial == loaded_.dial) {
      for (auto it = heard_.cbegin (); it != heard_.cend (); ++it)
        if (it.value () >= now - 1800 && it.value () <= now + 300) active.insert (it.key ());
    }
    if (!force && active == active_) return;
    active_ = active;
    QList<QTextEdit::ExtraSelection> selections;
    for (auto const& selection : display_->extraSelections ())
      if (!selection.format.property (MarkerProperty).toBool ()) selections.append (selection);
    markers_.clear ();
    if (!active.isEmpty ()) {
      QStringList alternatives;
      for (auto const& call : active) alternatives.append (QRegularExpression::escape (call));
      QRegularExpression pattern {"(?<![A-Z0-9/])(?:" + alternatives.join ('|') + ")(?![A-Z0-9/])"};
      for (auto block = display_->document ()->begin (); block.isValid (); block = block.next ()) {
        auto matches = pattern.globalMatch (block.text ());
        while (matches.hasNext ()) {
          auto match = matches.next ();
          QTextEdit::ExtraSelection selection;
          selection.cursor = QTextCursor {display_->document ()};
          selection.cursor.setPosition (block.position () + match.capturedStart ());
          selection.cursor.setPosition (block.position () + match.capturedEnd (), QTextCursor::KeepAnchor);
          selection.format.setProperty (MarkerProperty, true);
          selection.format.setBackground (QColor {"#7fe5df"});
          selection.format.setForeground (Qt::black);
          selection.format.setFontWeight (QFont::Bold);
          selection.format.setUnderlineStyle (QTextCharFormat::SingleUnderline);
          selection.format.setToolTip (tr ("PSK Reporter: %1 received your signal on this frequency and mode at %2 UTC.")
                                        .arg (match.captured (), QDateTime::fromSecsSinceEpoch (heard_.value (match.captured ()), Qt::UTC).toString ("HH:mm:ss")));
          markers_.append (selection);
          selections.append (selection);
        }
      }
    }
    display_->setExtraSelections (selections);
  }

  bool eventFilter (QObject *watched, QEvent *event) override
  {
    if (watched == display_->viewport () && event->type () == QEvent::ToolTip) {
      auto help = static_cast<QHelpEvent *> (event);
      auto position = display_->cursorForPosition (help->pos ()).position ();
      for (auto const& marker : markers_) {
        if (position >= marker.cursor.selectionStart () && position < marker.cursor.selectionEnd ()) {
          QToolTip::showText (help->globalPos (), marker.format.toolTip (), display_->viewport ());
          return true;
        }
      }
    }
    return QObject::eventFilter (watched, event);
  }

  QTextEdit *display_;
  PskReporterDialog::ContextProvider context_;
  PskReporterDialog::Context loaded_;
  HeardStations heard_;
  QSet<QString> active_;
  QList<QTextEdit::ExtraSelection> markers_;
  bool pending_ = false;
};
#endif
