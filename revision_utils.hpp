#ifndef REVISION_UTILS_HPP__
#define REVISION_UTILS_HPP__

#include <QString>

QString revision (QString const& svn_rev_string = QString {});
QString display_revision ();
QString version (bool include_patch = true);
inline QString custom_build_credit () { return QStringLiteral ("build by OE3SRO"); }
inline QString display_version () { return version () + QStringLiteral (" — ") + custom_build_credit (); }
QString program_title (QString const& revision = QString {});
QString http_user_agent ();

#endif
