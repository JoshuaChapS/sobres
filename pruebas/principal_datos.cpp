// Ejecutable de las pruebas de la capa de datos. Necesita un QCoreApplication
// vivo para que Qt encuentre el controlador QSQLITE.
#include <QCoreApplication>
#include <string>

#include "marco.h"

int main(int argc, char** argv) {
  QCoreApplication aplicacion(argc, argv);
  const std::string filtro = argc > 1 ? argv[1] : "";
  return marco::correrTodas(filtro);
}
