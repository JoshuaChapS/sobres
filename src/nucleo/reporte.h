// Sobres — reporte.h
// Reporte de un mes: en qué se fue el dinero, por categoría y por sobre, y
// cómo se compara contra el mes anterior.
//
// Solo los gastos cuentan como gasto. Los traspasos mueven dinero de un sobre
// a otro sin que salga de la cuenta, así que no aparecen; las entradas se
// reportan aparte.
#pragma once

#include <string>
#include <vector>

#include "modelos.h"

namespace sobres {

struct LineaReporte {
  Id id = kSinId;  // kSinId en la línea de "sin categoría"
  std::string nombre;
  std::string color = "#6b7280";
  Centavos mesActual = 0;
  Centavos mesAnterior = 0;
  Centavos diferencia = 0;  // actual menos anterior
  // Solo tiene sentido comparar en porcentaje si el mes anterior no fue cero.
  bool hayComparacion = false;
  double variacion = 0.0;  // 0.25 significa 25% más que el mes anterior
};

struct ReporteMensual {
  int anio = 0;
  int mes = 0;
  std::string titulo;  // "septiembre 2026"

  Centavos gastoTotal = 0;
  Centavos gastoTotalAnterior = 0;
  Centavos diferenciaGasto = 0;

  Centavos entradasTotal = 0;
  Centavos entradasTotalAnterior = 0;

  std::vector<LineaReporte> porCategoria;  // ordenadas de mayor a menor gasto
  std::vector<LineaReporte> porSobre;
};

ReporteMensual generarReporteMensual(int anio, int mes,
                                     const std::vector<Sobre>& sobres,
                                     const std::vector<Categoria>& categorias,
                                     const std::vector<Movimiento>& movimientos);

// Frase de comparación para el encabezado del reporte.
std::string resumirComparacion(const ReporteMensual& reporte);

}  // namespace sobres
