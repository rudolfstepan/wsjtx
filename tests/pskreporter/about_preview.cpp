#include "../../widgets/about.h"
#include "../../revision_utils.hpp"
#include <QApplication>
#include <QFontDatabase>
#include <QLabel>
#include <QDir>
#include <iostream>

int main (int argc, char **argv)
{
  QApplication app {argc, argv};
  QFontDatabase::addApplicationFont ("C:/Windows/Fonts/segoeui.ttf");
  app.setFont (QFont {"Segoe UI", 10});
  app.setApplicationName ("WSJT-X - Custom");
  app.setApplicationVersion (version ());
  CAboutDlg dialog;
  dialog.show ();
  app.processEvents ();
  QDir {}.mkpath (".local");
  if (!dialog.grab ().save (".local/about-preview.png")) return 1;
  std::cout << program_title (revision ()).toStdString () << '\n';
}
