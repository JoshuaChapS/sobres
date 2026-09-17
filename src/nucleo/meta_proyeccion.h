// Sobres — meta_proyeccion.h
//
// La matemática de una meta de ahorro es una anualidad con valor futuro. Se
// parte de un saldo que ya está ahorrado, se hacen aportes iguales al final de
// cada periodo y todo gana la misma tasa. La ecuación es:
//
//     VF = VP · (1+i)^n  +  A · [ ((1+i)^n − 1) / i ]
//
// donde VP es el saldo actual (valor presente), VF el monto objetivo (valor
// futuro), A el aporte por periodo, n el número de periodos que faltan e i la
// tasa efectiva del periodo.
//
// La tasa que captura el usuario es anual y efectiva. La del periodo se saca
// con equivalencia de tasas efectivas, i = (1+tasaAnual)^(1/m) − 1, no
// dividiendo entre m: dividir supondría capitalización simple y subestimaría
// los intereses.
#pragma once

#include <string>

#include "modelos.h"

namespace sobres {

enum class EstadoMeta {
  // El sobre ya tiene el monto objetivo o más.
  YaAlcanzada,
  // Con el aporte planeado sobra dinero en la fecha objetivo.
  Adelantado,
  // Con el aporte planeado se llega casi exacto (menos de un peso de
  // diferencia).
  EnLinea,
  // Con el aporte planeado no se llega.
  Atrasado,
  // Los datos no permiten proyectar (fecha ya pasada, tasa imposible).
  NoProyectable,
};

struct ProyeccionMeta {
  bool proyectable = false;
  // Si no es proyectable, explica por qué en una frase.
  std::string problema;

  long long periodos = 0;      // n
  double tasaPorPeriodo = 0.0; // i
  Centavos saldoActual = 0;    // VP
  Centavos montoObjetivo = 0;  // VF

  // Lo que habría que aportar cada periodo para llegar exacto. Nunca negativo:
  // si el saldo actual ya crece solo hasta rebasar el objetivo, es 0.
  Centavos aporteRequerido = 0;
  Centavos aportePlaneado = 0;
  // Planeado menos requerido. Positivo significa que sobra por periodo.
  Centavos diferenciaPorPeriodo = 0;

  // A dónde se llega en la fecha objetivo con el aporte planeado.
  Centavos valorFuturoProyectado = 0;
  // Proyectado menos objetivo. Positivo significa que sobra al final.
  Centavos diferenciaAlFinal = 0;

  EstadoMeta estado = EstadoMeta::NoProyectable;

  // Cuándo se alcanzaría el objetivo manteniendo el aporte planeado, sin
  // atarse a la fecha objetivo.
  bool tieneFechaEstimada = false;
  Fecha fechaEstimada;
  long long periodosHastaAlcanzar = 0;
};

// hoy es la fecha desde la que se cuentan los periodos que faltan.
ProyeccionMeta proyectarMeta(const Meta& meta, Centavos saldoActual,
                             const Fecha& hoy);

// Una o dos frases en español llano con el resultado, para mostrarlas junto a
// la meta.
std::string resumirProyeccion(const ProyeccionMeta& p, Periodicidad p_periodo);

}  // namespace sobres
