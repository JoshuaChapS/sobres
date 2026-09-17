// Sobres — marco.h
// Un marco de pruebas mínimo, sin dependencias. Se escribe una prueba así:
//
//   PRUEBA(dinero_formatea_miles) {
//     VERIFICAR_IGUAL(formatearPesos(123456), std::string("$1,234.56"));
//   }
//
// y el ejecutable de pruebas las corre todas y reporta las que fallan.
#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace marco {

struct Caso {
  std::string nombre;
  std::function<void()> funcion;
};

std::vector<Caso>& casos();

struct Registrador {
  Registrador(const std::string& nombre, std::function<void()> funcion) {
    casos().push_back({nombre, std::move(funcion)});
  }
};

// Excepción interna que lanza una aserción fallida.
struct Fallo {
  std::string mensaje;
};

void reportarFallo(const std::string& archivo, int linea,
                   const std::string& mensaje);

template <typename T>
std::string aTexto(const T& valor) {
  std::ostringstream flujo;
  flujo << valor;
  return flujo.str();
}

inline std::string aTexto(bool valor) { return valor ? "verdadero" : "falso"; }

int correrTodas(const std::string& filtro);

}  // namespace marco

#define PRUEBA(nombre)                                                     \
  static void prueba_##nombre();                                           \
  static marco::Registrador registro_##nombre(#nombre, prueba_##nombre);   \
  static void prueba_##nombre()

// Es variádico a propósito: así una condición con llaves y comas dentro, como
// VERIFICAR(f == (Fecha{2026, 1, 1})), no se parte en varios argumentos.
#define VERIFICAR(...)                                                     \
  do {                                                                     \
    if (!(__VA_ARGS__)) {                                                  \
      marco::reportarFallo(__FILE__, __LINE__,                             \
                           "no se cumplió: " #__VA_ARGS__);                \
    }                                                                      \
  } while (false)

#define VERIFICAR_IGUAL(obtenido, esperado)                                \
  do {                                                                     \
    const auto _obtenido = (obtenido);                                     \
    const auto _esperado = (esperado);                                     \
    if (!(_obtenido == _esperado)) {                                       \
      marco::reportarFallo(__FILE__, __LINE__,                             \
                           std::string(#obtenido) + " dio " +              \
                               marco::aTexto(_obtenido) + " y se esperaba " + \
                               marco::aTexto(_esperado));                  \
    }                                                                      \
  } while (false)

// Para comparar números con punto flotante.
#define VERIFICAR_CERCA(obtenido, esperado, tolerancia)                    \
  do {                                                                     \
    const double _o = static_cast<double>(obtenido);                       \
    const double _e = static_cast<double>(esperado);                       \
    const double _d = _o - _e < 0 ? _e - _o : _o - _e;                     \
    if (!(_d <= (tolerancia))) {                                           \
      marco::reportarFallo(__FILE__, __LINE__,                             \
                           std::string(#obtenido) + " dio " +              \
                               marco::aTexto(_o) + " y se esperaba " +     \
                               marco::aTexto(_e) + " con tolerancia " +    \
                               marco::aTexto(static_cast<double>(tolerancia))); \
    }                                                                      \
  } while (false)
