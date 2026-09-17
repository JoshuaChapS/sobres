#include "exportacion.h"

#include <algorithm>
#include <map>

namespace sobres {
namespace {

bool esAnteriorAlMes(const Fecha& f, int anio, int mes) {
  if (f.anio != anio) return f.anio < anio;
  return f.mes < mes;
}

bool esDelMes(const Fecha& f, int anio, int mes) {
  return f.anio == anio && f.mes == mes;
}

std::string nombreDeCategoria(const std::vector<Categoria>& categorias, Id id) {
  if (id == kSinId) return "Sin categoría";
  for (const Categoria& c : categorias) {
    if (c.id == id) return c.nombre;
  }
  return "Sin categoría";
}

std::string nombreDeSobre(const std::vector<Sobre>& sobres, Id id) {
  for (const Sobre& s : sobres) {
    if (s.id == id) return s.nombre;
  }
  return "(sobre eliminado)";
}

// Efecto de un movimiento sobre un sobre concreto, con su signo.
Centavos efectoSobre(const Movimiento& m, Id sobre) {
  switch (m.tipo) {
    case TipoMovimiento::Entrada:
      return m.sobreOrigen == sobre ? m.monto : 0;
    case TipoMovimiento::Gasto:
      return m.sobreOrigen == sobre ? -m.monto : 0;
    case TipoMovimiento::Traspaso:
      if (m.sobreDestino == sobre) return m.monto;
      if (m.sobreOrigen == sobre) return -m.monto;
      return 0;
  }
  return 0;
}

}  // namespace

std::string nombreDeArchivoSugerido(int anio, int mes) {
  std::string texto = "Sobres " + std::to_string(anio) + "-";
  if (mes < 10) texto += "0";
  texto += std::to_string(mes) + ".xlsx";
  return texto;
}

ReporteMensualPorSobre generarReporteMensualPorSobre(
    int anio, int mes, const std::vector<Sobre>& sobres,
    const std::vector<Categoria>& categorias,
    const std::vector<Movimiento>& movimientos,
    const std::vector<Pasivo>& pasivos) {
  ReporteMensualPorSobre reporte;
  reporte.anio = anio;
  reporte.mes = mes;
  reporte.titulo = nombreDeMes(mes) + " " + std::to_string(anio);
  reporte.porCategoria =
      generarReporteMensual(anio, mes, sobres, categorias, movimientos);

  // Los movimientos del mes, en orden cronológico. El identificador desempata
  // los del mismo día, que es el orden en que se capturaron.
  std::vector<Movimiento> delMes;
  for (const Movimiento& m : movimientos) {
    if (esDelMes(m.fecha, anio, mes)) delMes.push_back(m);
  }
  std::sort(delMes.begin(), delMes.end(),
            [](const Movimiento& a, const Movimiento& b) {
              if (a.fecha != b.fecha) return a.fecha < b.fecha;
              return a.id < b.id;
            });

  std::map<Id, Centavos> saldosAlCierre;

  for (const Sobre& s : sobres) {
    ReporteDeSobre linea;
    linea.id = s.id;
    linea.nombre = s.nombre;
    linea.tipo = s.tipo;

    // Saldo al inicio: todo lo que pasó antes de este mes.
    for (const Movimiento& m : movimientos) {
      if (!esAnteriorAlMes(m.fecha, anio, mes)) continue;
      linea.saldoInicial += efectoSobre(m, s.id);
    }

    Centavos corriente = linea.saldoInicial;
    for (const Movimiento& m : delMes) {
      const Centavos efecto = efectoSobre(m, s.id);
      if (efecto == 0) continue;

      MovimientoDeSobre fila;
      fila.fecha = m.fecha;
      fila.nota = m.nota;
      fila.efecto = efecto;
      corriente += efecto;
      fila.saldoCorriente = corriente;

      switch (m.tipo) {
        case TipoMovimiento::Entrada:
          fila.concepto = "Entrada";
          fila.categoria = nombreDeCategoria(categorias, m.categoria);
          linea.entradas += m.monto;
          break;
        case TipoMovimiento::Gasto:
          fila.concepto = "Gasto";
          fila.categoria = nombreDeCategoria(categorias, m.categoria);
          linea.gastos += m.monto;
          break;
        case TipoMovimiento::Traspaso:
          if (efecto > 0) {
            fila.concepto =
                "Traspaso desde " + nombreDeSobre(sobres, m.sobreOrigen);
            linea.traspasosRecibidos += m.monto;
          } else {
            fila.concepto =
                "Traspaso hacia " + nombreDeSobre(sobres, m.sobreDestino);
            linea.traspasosEnviados += m.monto;
          }
          break;
      }
      linea.movimientos.push_back(fila);
    }

    linea.saldoFinal = corriente;
    saldosAlCierre[s.id] = linea.saldoFinal;

    // Un sobre archivado sin movimientos en el mes y sin saldo no aporta nada
    // al reporte; los activos siempre aparecen, aunque hayan estado quietos.
    const bool aporta =
        !linea.movimientos.empty() || linea.saldoFinal != 0 ||
        linea.saldoInicial != 0;
    if (s.archivado && !aporta) continue;

    reporte.enLaCuentaAlInicio += linea.saldoInicial;
    reporte.sobres.push_back(linea);
  }

  reporte.patrimonioAlCierre =
      calcularPatrimonio(sobres, saldosAlCierre, pasivos);
  return reporte;
}

namespace {

using xlsx::Celda;

std::vector<Celda> filaDeTexto(const std::vector<std::string>& textos,
                               bool negrita) {
  std::vector<Celda> fila;
  fila.reserve(textos.size());
  for (const std::string& t : textos) fila.push_back(xlsx::texto(t, negrita));
  return fila;
}

xlsx::Hoja construirResumen(const ReporteMensualPorSobre& reporte) {
  xlsx::Hoja hoja;
  hoja.nombre = "Resumen";
  hoja.anchosDeColumna = {28, 14, 15, 14, 14, 16, 16, 15};

  hoja.filas.push_back({xlsx::texto("Sobres — reporte de " + reporte.titulo,
                                    true)});
  hoja.filas.push_back({});

  hoja.filas.push_back(filaDeTexto({"Sobre", "Tipo", "Saldo inicial",
                                    "Entradas", "Gastos",
                                    "Traspasos recibidos",
                                    "Traspasos enviados", "Saldo final"},
                                   true));

  Centavos totalInicial = 0, totalEntradas = 0, totalGastos = 0;
  Centavos totalRecibido = 0, totalEnviado = 0, totalFinal = 0;

  for (const ReporteDeSobre& s : reporte.sobres) {
    hoja.filas.push_back({
        xlsx::texto(s.nombre),
        xlsx::texto(nombreDeTipo(s.tipo)),
        xlsx::dinero(s.saldoInicial),
        xlsx::dinero(s.entradas),
        xlsx::dinero(s.gastos),
        xlsx::dinero(s.traspasosRecibidos),
        xlsx::dinero(s.traspasosEnviados),
        xlsx::dinero(s.saldoFinal),
    });
    totalInicial += s.saldoInicial;
    totalEntradas += s.entradas;
    totalGastos += s.gastos;
    totalRecibido += s.traspasosRecibidos;
    totalEnviado += s.traspasosEnviados;
    totalFinal += s.saldoFinal;
  }

  hoja.filas.push_back({
      xlsx::texto("Total", true),
      xlsx::vacia(),
      xlsx::dinero(totalInicial, true),
      xlsx::dinero(totalEntradas, true),
      xlsx::dinero(totalGastos, true),
      xlsx::dinero(totalRecibido, true),
      xlsx::dinero(totalEnviado, true),
      xlsx::dinero(totalFinal, true),
  });

  hoja.filas.push_back({});
  hoja.filas.push_back({xlsx::texto("Al cierre del mes", true)});
  const ResumenPatrimonio& p = reporte.patrimonioAlCierre;
  hoja.filas.push_back({xlsx::texto("En la cuenta"), xlsx::vacia(),
                        xlsx::dinero(p.enLaCuenta)});
  hoja.filas.push_back({xlsx::texto("Dinero de terceros"), xlsx::vacia(),
                        xlsx::dinero(p.dineroAjeno)});
  hoja.filas.push_back({xlsx::texto("Dinero propio"), xlsx::vacia(),
                        xlsx::dinero(p.dineroPropio)});
  hoja.filas.push_back({xlsx::texto("Pasivos registrados hoy"), xlsx::vacia(),
                        xlsx::dinero(p.pasivos)});
  hoja.filas.push_back({xlsx::texto("Patrimonio real", true), xlsx::vacia(),
                        xlsx::dinero(p.patrimonioReal, true)});
  hoja.filas.push_back(
      {xlsx::texto("El saldo de cada pasivo es el que está registrado hoy en "
                   "la aplicación, no el que tenía al cierre del mes.")});

  hoja.filas.push_back({});
  hoja.filas.push_back({xlsx::texto("Gasto por categoría", true)});
  hoja.filas.push_back(
      filaDeTexto({"Categoría", "Este mes", "Mes anterior", "Diferencia"},
                  true));
  for (const LineaReporte& l : reporte.porCategoria.porCategoria) {
    hoja.filas.push_back({
        xlsx::texto(l.nombre),
        xlsx::dinero(l.mesActual),
        xlsx::dinero(l.mesAnterior),
        xlsx::dinero(l.diferencia),
    });
  }
  hoja.filas.push_back({
      xlsx::texto("Total", true),
      xlsx::dinero(reporte.porCategoria.gastoTotal, true),
      xlsx::dinero(reporte.porCategoria.gastoTotalAnterior, true),
      xlsx::dinero(reporte.porCategoria.diferenciaGasto, true),
  });

  return hoja;
}

xlsx::Hoja construirHojaDeSobre(const ReporteDeSobre& sobre,
                                const std::string& titulo,
                                const std::vector<std::string>& nombresUsados) {
  xlsx::Hoja hoja;
  hoja.nombre = xlsx::nombreDeHojaValido(sobre.nombre, nombresUsados);
  hoja.anchosDeColumna = {12, 30, 20, 34, 14, 14};

  hoja.filas.push_back({xlsx::texto(sobre.nombre + " — " + titulo, true)});
  hoja.filas.push_back({xlsx::texto(nombreDeTipo(sobre.tipo))});
  hoja.filas.push_back({});
  hoja.filas.push_back({xlsx::texto("Saldo al inicio del mes"), xlsx::vacia(),
                        xlsx::vacia(), xlsx::vacia(), xlsx::vacia(),
                        xlsx::dinero(sobre.saldoInicial)});
  hoja.filas.push_back({});

  hoja.filas.push_back(filaDeTexto(
      {"Fecha", "Concepto", "Categoría", "Nota", "Movimiento", "Saldo"}, true));

  if (sobre.movimientos.empty()) {
    hoja.filas.push_back({xlsx::vacia(),
                          xlsx::texto("Sin movimientos en este mes")});
  }
  for (const MovimientoDeSobre& m : sobre.movimientos) {
    hoja.filas.push_back({
        xlsx::fecha(m.fecha),
        xlsx::texto(m.concepto),
        xlsx::texto(m.categoria),
        xlsx::texto(m.nota),
        xlsx::dinero(m.efecto),
        xlsx::dinero(m.saldoCorriente),
    });
  }

  hoja.filas.push_back({});
  hoja.filas.push_back({xlsx::texto("Entradas del mes"), xlsx::vacia(),
                        xlsx::vacia(), xlsx::vacia(),
                        xlsx::dinero(sobre.entradas)});
  hoja.filas.push_back({xlsx::texto("Gastos del mes"), xlsx::vacia(),
                        xlsx::vacia(), xlsx::vacia(),
                        xlsx::dinero(sobre.gastos)});
  hoja.filas.push_back({xlsx::texto("Traspasos recibidos"), xlsx::vacia(),
                        xlsx::vacia(), xlsx::vacia(),
                        xlsx::dinero(sobre.traspasosRecibidos)});
  hoja.filas.push_back({xlsx::texto("Traspasos enviados"), xlsx::vacia(),
                        xlsx::vacia(), xlsx::vacia(),
                        xlsx::dinero(sobre.traspasosEnviados)});
  hoja.filas.push_back({xlsx::texto("Saldo al cierre del mes", true),
                        xlsx::vacia(), xlsx::vacia(), xlsx::vacia(),
                        xlsx::vacia(), xlsx::dinero(sobre.saldoFinal, true)});
  return hoja;
}

}  // namespace

xlsx::Libro construirLibroDeReporte(const ReporteMensualPorSobre& reporte) {
  xlsx::Libro libro;
  libro.hojas.push_back(construirResumen(reporte));

  std::vector<std::string> usados = {libro.hojas.front().nombre};
  for (const ReporteDeSobre& s : reporte.sobres) {
    xlsx::Hoja hoja = construirHojaDeSobre(s, reporte.titulo, usados);
    usados.push_back(hoja.nombre);
    libro.hojas.push_back(std::move(hoja));
  }
  return libro;
}

}  // namespace sobres
