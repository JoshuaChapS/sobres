#include "marco.h"

#include <algorithm>

namespace marco {

std::vector<Caso>& casos() {
  static std::vector<Caso> lista;
  return lista;
}

void reportarFallo(const std::string& archivo, int linea,
                   const std::string& mensaje) {
  throw Fallo{archivo + ":" + std::to_string(linea) + " — " + mensaje};
}

int correrTodas(const std::string& filtro) {
  int corridas = 0;
  int fallidas = 0;
  auto& lista = casos();
  std::stable_sort(lista.begin(), lista.end(),
                   [](const Caso& a, const Caso& b) { return a.nombre < b.nombre; });

  for (const Caso& caso : lista) {
    if (!filtro.empty() && caso.nombre.find(filtro) == std::string::npos) {
      continue;
    }
    ++corridas;
    try {
      caso.funcion();
    } catch (const Fallo& f) {
      ++fallidas;
      std::cout << "FALLA  " << caso.nombre << "\n       " << f.mensaje << "\n";
      continue;
    } catch (const std::exception& e) {
      ++fallidas;
      std::cout << "FALLA  " << caso.nombre
                << "\n       excepción inesperada: " << e.what() << "\n";
      continue;
    }
  }

  std::cout << "\n" << corridas << " pruebas, " << fallidas << " fallidas\n";
  return fallidas == 0 ? 0 : 1;
}

}  // namespace marco
