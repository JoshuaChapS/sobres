// Sobres — exportacion.h
// El reporte mensual visto sobre por sobre: con qué saldo empezó el mes cada
// uno, qué movimientos tuvo y con qué saldo cerró. De aquí sale el archivo de
// Excel.
#pragma once

#include <string>
#include <vector>

#include "modelos.h"
#include "reporte.h"
#include "saldos.h"
#include "xlsx.h"

namespace sobres {

// Un movimiento tal como se ve desde un sobre concreto: ya con su efecto
// firmado sobre ese sobre y con el saldo que dejó.
struct MovimientoDeSobre {
  Fecha fecha;
  std::string concepto;   // "Entrada", "Gasto", "Traspaso desde Casa"…
  std::string categoria;
  std::string nota;
  Centavos efecto = 0;         // positivo si entra, negativo si sale
  Centavos saldoCorriente = 0; // saldo del sobre después de este movimiento
};

struct ReporteDeSobre {
  Id id = kSinId;
  std::string nombre;
  TipoSobre tipo = TipoSobre::Propio;

  Centavos saldoInicial = 0;
  Centavos entradas = 0;
  Centavos gastos = 0;
  Centavos traspasosRecibidos = 0;
  Centavos traspasosEnviados = 0;
  Centavos saldoFinal = 0;

  std::vector<MovimientoDeSobre> movimientos;  // en orden cronológico
};

struct ReporteMensualPorSobre {
  int anio = 0;
  int mes = 0;
  std::string titulo;  // "septiembre 2026"

  std::vector<ReporteDeSobre> sobres;

  // Fotografía de la cuenta al cierre del mes. Los pasivos son los que están
  // registrados hoy: la aplicación guarda el saldo actual de cada deuda, no su
  // historia, y el reporte lo dice así en la hoja.
  ResumenPatrimonio patrimonioAlCierre;
  Centavos enLaCuentaAlInicio = 0;

  // El desglose por categoría contra el mes anterior, que ya calculaba la
  // aplicación.
  ReporteMensual porCategoria;
};

ReporteMensualPorSobre generarReporteMensualPorSobre(
    int anio, int mes, const std::vector<Sobre>& sobres,
    const std::vector<Categoria>& categorias,
    const std::vector<Movimiento>& movimientos,
    const std::vector<Pasivo>& pasivos);

// Arma el libro de Excel: una hoja de resumen y una hoja por cada sobre.
xlsx::Libro construirLibroDeReporte(const ReporteMensualPorSobre& reporte);

// Nombre sugerido para el archivo, por ejemplo "Sobres 2026-09.xlsx".
std::string nombreDeArchivoSugerido(int anio, int mes);

}  // namespace sobres
