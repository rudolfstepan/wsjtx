#ifndef WSJTX_LOOPBACK_INPUT_HPP
#define WSJTX_LOOPBACK_INPUT_HPP
#include <QCoreApplication>
#include <QVariant>
#include <QString>

inline QString loopbackInputName ()
{
  return QCoreApplication::instance ()->property ("wasapiLoopbackDevice").toString ();
}
#endif
