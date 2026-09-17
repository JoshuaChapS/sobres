// Ejecutable de las pruebas del núcleo. No enlaza con Qt: el núcleo no lo
// necesita, y eso mantiene la lógica financiera comprobable con solo un
// compilador de C++.
#include <string>

#include "marco.h"

int main(int argc, char** argv) {
  const std::string filtro = argc > 1 ? argv[1] : "";
  return marco::correrTodas(filtro);
}
