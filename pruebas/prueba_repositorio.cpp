#include "../src/datos/repositorio.h"

#include <QDir>
#include <QTemporaryDir>

#include "../src/nucleo/saldos.h"
#include "marco.h"

using namespace sobres;

namespace {

// Cada prueba trabaja sobre un archivo nuevo en una carpeta temporal, así
// ninguna arrastra datos de otra.
struct BaseTemporal {
  QTemporaryDir carpeta;
  Repositorio repositorio;
  QString error;

  BaseTemporal() {
    const QString ruta = QDir(carpeta.path()).filePath(QStringLiteral("prueba.db"));
    if (!repositorio.abrir(ruta, &error)) {
      marco::reportarFallo(__FILE__, __LINE__,
                           "no se pudo abrir la base: " + error.toStdString());
    }
  }
};

Sobre nuevoSobre(const char* nombre, TipoSobre tipo) {
  Sobre s;
  s.nombre = nombre;
  s.tipo = tipo;
  s.color = "#3b82f6";
  return s;
}

}  // namespace

PRUEBA(repositorio_arranca_vacio) {
  BaseTemporal b;
  VERIFICAR(b.repositorio.estaAbierto());
  VERIFICAR(b.repositorio.estaVacia());
  VERIFICAR(b.repositorio.sobres().empty());
  VERIFICAR(b.repositorio.categorias().empty());
  VERIFICAR(b.repositorio.movimientos().empty());
  VERIFICAR(b.repositorio.pasivos().empty());
  VERIFICAR(b.repositorio.metas().empty());
  VERIFICAR(b.repositorio.recurrentes().empty());
  VERIFICAR(b.repositorio.plantillas().empty());
}

PRUEBA(repositorio_guarda_y_recupera_un_sobre) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Gasto diario", TipoSobre::Propio);
  VERIFICAR(b.repositorio.guardarSobre(s, &b.error));
  VERIFICAR(s.id != kSinId);
  VERIFICAR(!b.repositorio.estaVacia());

  const auto lista = b.repositorio.sobres();
  VERIFICAR_IGUAL(lista.size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(lista[0].nombre, std::string("Gasto diario"));
  VERIFICAR(lista[0].tipo == TipoSobre::Propio);

  s.nombre = "Diario";
  VERIFICAR(b.repositorio.guardarSobre(s, &b.error));
  VERIFICAR_IGUAL(b.repositorio.sobres().size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(b.repositorio.sobre(s.id)->nombre, std::string("Diario"));
}

PRUEBA(repositorio_ordena_y_reordena_sobres) {
  BaseTemporal b;
  Sobre a = nuevoSobre("Primero", TipoSobre::Propio);
  Sobre c = nuevoSobre("Segundo", TipoSobre::Propio);
  Sobre d = nuevoSobre("Tercero", TipoSobre::Propio);
  b.repositorio.guardarSobre(a, &b.error);
  b.repositorio.guardarSobre(c, &b.error);
  b.repositorio.guardarSobre(d, &b.error);

  VERIFICAR(b.repositorio.moverSobre(d.id, -1, &b.error));
  auto lista = b.repositorio.sobres();
  VERIFICAR_IGUAL(lista[1].nombre, std::string("Tercero"));
  VERIFICAR_IGUAL(lista[2].nombre, std::string("Segundo"));

  // Mover más allá del extremo no hace nada y no es un error.
  VERIFICAR(b.repositorio.moverSobre(lista[0].id, -1, &b.error));
  VERIFICAR_IGUAL(b.repositorio.sobres()[0].nombre, std::string("Primero"));
}

PRUEBA(repositorio_archiva_en_lugar_de_borrar_lo_que_tiene_historia) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Gasto diario", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);

  Movimiento m;
  m.fecha = Fecha{2026, 9, 1};
  m.tipo = TipoMovimiento::Entrada;
  m.sobreOrigen = s.id;
  m.monto = 100000;
  VERIFICAR(b.repositorio.guardarMovimiento(m, &b.error));

  QString problema;
  VERIFICAR(!b.repositorio.eliminarSobre(s.id, &problema));
  VERIFICAR(problema.contains(QStringLiteral("Archívalo")));

  VERIFICAR(b.repositorio.archivarSobre(s.id, true, &b.error));
  VERIFICAR(b.repositorio.sobres(false).empty());
  VERIFICAR_IGUAL(b.repositorio.sobres(true).size(),
                  static_cast<std::size_t>(1));
}

PRUEBA(repositorio_borra_un_sobre_sin_movimientos) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Me equivoqué", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);
  VERIFICAR(b.repositorio.eliminarSobre(s.id, &b.error));
  VERIFICAR(b.repositorio.sobres(true).empty());
}

PRUEBA(repositorio_conserva_los_montos_en_centavos_exactos) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Gasto diario", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);

  // Un monto grande que un double no podría representar sin perder centavos.
  Movimiento m;
  m.fecha = Fecha{2026, 9, 1};
  m.tipo = TipoMovimiento::Entrada;
  m.sobreOrigen = s.id;
  m.monto = 9007199254740993LL;
  VERIFICAR(b.repositorio.guardarMovimiento(m, &b.error));
  VERIFICAR_IGUAL(b.repositorio.movimientos()[0].monto, m.monto);
}

PRUEBA(repositorio_guarda_traspasos_con_destino_y_los_demas_sin_el) {
  BaseTemporal b;
  Sobre a = nuevoSobre("Diario", TipoSobre::Propio);
  Sobre c = nuevoSobre("Ahorro", TipoSobre::Meta);
  b.repositorio.guardarSobre(a, &b.error);
  b.repositorio.guardarSobre(c, &b.error);

  Movimiento traspaso;
  traspaso.fecha = Fecha{2026, 9, 2};
  traspaso.tipo = TipoMovimiento::Traspaso;
  traspaso.sobreOrigen = a.id;
  traspaso.sobreDestino = c.id;
  traspaso.monto = 50000;
  VERIFICAR(b.repositorio.guardarMovimiento(traspaso, &b.error));

  Movimiento gasto;
  gasto.fecha = Fecha{2026, 9, 3};
  gasto.tipo = TipoMovimiento::Gasto;
  gasto.sobreOrigen = a.id;
  gasto.sobreDestino = c.id;  // se ignora por no ser traspaso
  gasto.monto = 10000;
  VERIFICAR(b.repositorio.guardarMovimiento(gasto, &b.error));

  const auto lista = b.repositorio.movimientos();
  VERIFICAR_IGUAL(lista.size(), static_cast<std::size_t>(2));
  // Vienen del más reciente al más antiguo.
  VERIFICAR(lista[0].tipo == TipoMovimiento::Gasto);
  VERIFICAR_IGUAL(lista[0].sobreDestino, kSinId);
  VERIFICAR_IGUAL(lista[1].sobreDestino, c.id);
}

PRUEBA(repositorio_filtra_movimientos_por_rango_de_fechas) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Diario", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);

  for (int dia : {1, 15, 30}) {
    Movimiento m;
    m.fecha = Fecha{2026, 9, dia};
    m.tipo = TipoMovimiento::Gasto;
    m.sobreOrigen = s.id;
    m.monto = 1000;
    b.repositorio.guardarMovimiento(m, &b.error);
  }
  Movimiento otroMes;
  otroMes.fecha = Fecha{2026, 10, 1};
  otroMes.tipo = TipoMovimiento::Gasto;
  otroMes.sobreOrigen = s.id;
  otroMes.monto = 1000;
  b.repositorio.guardarMovimiento(otroMes, &b.error);

  const auto septiembre = b.repositorio.movimientosEntre(Fecha{2026, 9, 1},
                                                         Fecha{2026, 9, 30});
  VERIFICAR_IGUAL(septiembre.size(), static_cast<std::size_t>(3));
}

PRUEBA(repositorio_guarda_varios_movimientos_o_ninguno) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Diario", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);

  std::vector<Movimiento> lote;
  for (int k = 0; k < 3; ++k) {
    Movimiento m;
    m.fecha = Fecha{2026, 9, 1};
    m.tipo = TipoMovimiento::Entrada;
    m.sobreOrigen = s.id;
    m.monto = 1000 * (k + 1);
    lote.push_back(m);
  }
  // El último apunta a un sobre inexistente: la llave foránea lo rechaza y no
  // debe quedar ninguno guardado.
  lote.back().sobreOrigen = 9999;

  QString problema;
  VERIFICAR(!b.repositorio.guardarMovimientos(lote, &problema));
  VERIFICAR(b.repositorio.movimientos().empty());

  lote.back().sobreOrigen = s.id;
  for (Movimiento& m : lote) m.id = kSinId;
  VERIFICAR(b.repositorio.guardarMovimientos(lote, &b.error));
  VERIFICAR_IGUAL(b.repositorio.movimientos().size(),
                  static_cast<std::size_t>(3));
}

PRUEBA(repositorio_deja_sin_categoria_los_movimientos_al_borrarla) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Diario", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);
  Categoria c;
  c.nombre = "Comida";
  c.color = "#ef4444";
  VERIFICAR(b.repositorio.guardarCategoria(c, &b.error));

  Movimiento m;
  m.fecha = Fecha{2026, 9, 1};
  m.tipo = TipoMovimiento::Gasto;
  m.sobreOrigen = s.id;
  m.categoria = c.id;
  m.monto = 20000;
  b.repositorio.guardarMovimiento(m, &b.error);

  VERIFICAR(b.repositorio.eliminarCategoria(c.id, &b.error));
  const auto lista = b.repositorio.movimientos();
  VERIFICAR_IGUAL(lista.size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(lista[0].categoria, kSinId);
}

PRUEBA(repositorio_guarda_pasivos_y_metas) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Viaje", TipoSobre::Meta);
  b.repositorio.guardarSobre(s, &b.error);

  Pasivo tarjeta;
  tarjeta.nombre = "Tarjeta de crédito";
  tarjeta.saldo = 450000;
  VERIFICAR(b.repositorio.guardarPasivo(tarjeta, &b.error));
  VERIFICAR_IGUAL(b.repositorio.pasivos()[0].saldo, static_cast<Centavos>(450000));

  Meta meta;
  meta.nombre = "Japón";
  meta.montoObjetivo = 10000000;
  meta.fechaObjetivo = Fecha{2029, 3, 1};
  meta.sobreAsociado = s.id;
  meta.aportePlaneado = 300000;
  meta.periodicidad = Periodicidad::Quincenal;
  meta.tasaAnual = 0.085;
  VERIFICAR(b.repositorio.guardarMeta(meta, &b.error));

  const auto metas = b.repositorio.metas();
  VERIFICAR_IGUAL(metas.size(), static_cast<std::size_t>(1));
  VERIFICAR(metas[0].fechaObjetivo == (Fecha{2029, 3, 1}));
  VERIFICAR(metas[0].periodicidad == Periodicidad::Quincenal);
  VERIFICAR_CERCA(metas[0].tasaAnual, 0.085, 1e-12);
}

PRUEBA(repositorio_materializa_un_recurrente_y_adelanta_la_fecha) {
  BaseTemporal b;
  Sobre s = nuevoSobre("Casa", TipoSobre::Propio);
  b.repositorio.guardarSobre(s, &b.error);

  Recurrente r;
  r.nombre = "Renta";
  r.tipo = TipoMovimiento::Gasto;
  r.sobreOrigen = s.id;
  r.monto = 800000;
  r.periodicidad = Periodicidad::Mensual;
  r.proximaFecha = Fecha{2026, 9, 1};
  VERIFICAR(b.repositorio.guardarRecurrente(r, &b.error));

  VERIFICAR(b.repositorio.materializarRecurrente(r.id, Fecha{2026, 9, 2},
                                                 &b.error));
  const auto movimientos = b.repositorio.movimientos();
  VERIFICAR_IGUAL(movimientos.size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(movimientos[0].monto, static_cast<Centavos>(800000));
  VERIFICAR(movimientos[0].fecha == (Fecha{2026, 9, 2}));

  // La próxima fecha avanza desde la programada, no desde la fecha real.
  VERIFICAR(b.repositorio.recurrentes()[0].proximaFecha ==
            (Fecha{2026, 10, 1}));
}

PRUEBA(repositorio_guarda_plantillas_con_sus_lineas) {
  BaseTemporal b;
  Sobre a = nuevoSobre("Diario", TipoSobre::Propio);
  Sobre c = nuevoSobre("Casa", TipoSobre::Propio);
  b.repositorio.guardarSobre(a, &b.error);
  b.repositorio.guardarSobre(c, &b.error);

  PlantillaReparto p;
  p.nombre = "Sueldo quincenal";
  p.modo = ModoReparto::Porcentajes;
  p.montoSugerido = 1200000;
  p.lineas = {{a.id, 6000, false}, {c.id, 4000, true}};
  VERIFICAR(b.repositorio.guardarPlantilla(p, &b.error));

  auto lista = b.repositorio.plantillas();
  VERIFICAR_IGUAL(lista.size(), static_cast<std::size_t>(1));
  VERIFICAR_IGUAL(lista[0].lineas.size(), static_cast<std::size_t>(2));
  VERIFICAR_IGUAL(lista[0].lineas[0].valor, static_cast<std::int64_t>(6000));
  VERIFICAR(lista[0].lineas[1].recibeResto);

  // Al reeditar, las líneas se reemplazan en vez de duplicarse.
  p.lineas = {{a.id, 10000, false}};
  VERIFICAR(b.repositorio.guardarPlantilla(p, &b.error));
  lista = b.repositorio.plantillas();
  VERIFICAR_IGUAL(lista[0].lineas.size(), static_cast<std::size_t>(1));

  VERIFICAR(b.repositorio.eliminarPlantilla(p.id, &b.error));
  VERIFICAR(b.repositorio.plantillas().empty());
}

PRUEBA(repositorio_sobrevive_a_cerrar_y_volver_a_abrir) {
  QTemporaryDir carpeta;
  const QString ruta = QDir(carpeta.path()).filePath(QStringLiteral("datos.db"));
  Id idSobre = kSinId;
  QString error;
  {
    Repositorio repo;
    VERIFICAR(repo.abrir(ruta, &error));
    Sobre s = nuevoSobre("Gasto diario", TipoSobre::Propio);
    VERIFICAR(repo.guardarSobre(s, &error));
    idSobre = s.id;

    Movimiento m;
    m.fecha = Fecha{2026, 9, 1};
    m.tipo = TipoMovimiento::Entrada;
    m.sobreOrigen = s.id;
    m.monto = 123456;
    VERIFICAR(repo.guardarMovimiento(m, &error));
  }
  {
    Repositorio repo;
    VERIFICAR(repo.abrir(ruta, &error));
    const auto sobresGuardados = repo.sobres();
    VERIFICAR_IGUAL(sobresGuardados.size(), static_cast<std::size_t>(1));
    VERIFICAR_IGUAL(sobresGuardados[0].id, idSobre);
    const auto saldos = calcularSaldos(sobresGuardados, repo.movimientos());
    VERIFICAR_IGUAL(saldos.at(idSobre), static_cast<Centavos>(123456));
  }
}

PRUEBA(repositorio_calcula_el_patrimonio_de_punta_a_punta) {
  BaseTemporal b;
  Sobre diario = nuevoSobre("Gasto diario", TipoSobre::Propio);
  Sobre ajeno = nuevoSobre("Renta de los roomies", TipoSobre::Ajeno);
  b.repositorio.guardarSobre(diario, &b.error);
  b.repositorio.guardarSobre(ajeno, &b.error);

  Movimiento entradaPropia;
  entradaPropia.fecha = Fecha{2026, 9, 1};
  entradaPropia.tipo = TipoMovimiento::Entrada;
  entradaPropia.sobreOrigen = diario.id;
  entradaPropia.monto = 1000000;
  b.repositorio.guardarMovimiento(entradaPropia, &b.error);

  Movimiento entradaAjena;
  entradaAjena.fecha = Fecha{2026, 9, 1};
  entradaAjena.tipo = TipoMovimiento::Entrada;
  entradaAjena.sobreOrigen = ajeno.id;
  entradaAjena.monto = 600000;
  b.repositorio.guardarMovimiento(entradaAjena, &b.error);

  Pasivo tarjeta;
  tarjeta.nombre = "Tarjeta";
  tarjeta.saldo = 350000;
  b.repositorio.guardarPasivo(tarjeta, &b.error);

  const auto lista = b.repositorio.sobres();
  const auto saldos = calcularSaldos(lista, b.repositorio.movimientos());
  const ResumenPatrimonio r =
      calcularPatrimonio(lista, saldos, b.repositorio.pasivos());

  VERIFICAR_IGUAL(r.enLaCuenta, static_cast<Centavos>(1600000));
  VERIFICAR_IGUAL(r.patrimonioReal, static_cast<Centavos>(650000));
}
