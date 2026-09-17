#include "../src/nucleo/meta_proyeccion.h"

#include <cmath>

#include "marco.h"

using namespace sobres;

namespace {

Meta metaBase() {
  Meta m;
  m.id = 1;
  m.nombre = "Viaje a Japón";
  m.montoObjetivo = 10000000;  // $100,000.00
  m.fechaObjetivo = Fecha{2028, 1, 1};
  m.sobreAsociado = 3;
  m.periodicidad = Periodicidad::Mensual;
  m.tasaAnual = 0.08;
  m.aportePlaneado = 300000;  // $3,000.00
  return m;
}

const Fecha kHoy{2026, 1, 1};

}  // namespace

PRUEBA(meta_convierte_la_tasa_anual_a_tasa_del_periodo) {
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  // Equivalencia de tasas efectivas: (1+i)^12 = 1.08.
  VERIFICAR_CERCA(std::pow(1.0 + p.tasaPorPeriodo, 12) - 1.0, 0.08, 1e-12);
  VERIFICAR_CERCA(p.tasaPorPeriodo, 0.00643403011000343, 1e-15);
}

PRUEBA(meta_cuenta_los_periodos_que_faltan) {
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR_IGUAL(p.periodos, 24LL);
}

PRUEBA(meta_calcula_el_aporte_requerido_de_la_anualidad) {
  // Valores calculados aparte con la fórmula de la anualidad con valor futuro:
  // A = (VF − VP·(1+i)^n) · i / ((1+i)^n − 1), con VP = $20,000.00,
  // VF = $100,000.00, tasa anual 8% efectiva, 24 periodos mensuales.
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(p.proyectable);
  VERIFICAR_IGUAL(p.aporteRequerido, static_cast<Centavos>(296460));
}

PRUEBA(meta_compara_el_aporte_planeado_contra_el_requerido) {
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  // Planea $3,000.00 y necesita $2,964.60: va adelantado por $35.40 al mes.
  VERIFICAR_IGUAL(p.diferenciaPorPeriodo, static_cast<Centavos>(3540));
  VERIFICAR_IGUAL(p.valorFuturoProyectado, static_cast<Centavos>(10091545));
  VERIFICAR_IGUAL(p.diferenciaAlFinal, static_cast<Centavos>(91545));
  VERIFICAR(p.estado == EstadoMeta::Adelantado);
}

PRUEBA(meta_detecta_el_atraso) {
  Meta m = metaBase();
  m.aportePlaneado = 200000;  // $2,000.00, muy por debajo de lo necesario
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(p.estado == EstadoMeta::Atrasado);
  VERIFICAR(p.diferenciaAlFinal < 0);
  VERIFICAR(p.diferenciaPorPeriodo < 0);
  // Con menos aporte, la fecha estimada cae después de la fecha objetivo.
  VERIFICAR(p.tieneFechaEstimada);
  VERIFICAR(m.fechaObjetivo < p.fechaEstimada);
}

PRUEBA(meta_aportando_lo_requerido_llega_casi_exacto) {
  Meta m = metaBase();
  const ProyeccionMeta previa = proyectarMeta(m, 2000000, kHoy);
  m.aportePlaneado = previa.aporteRequerido;
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(p.estado == EstadoMeta::EnLinea);
  // El redondeo del aporte al centavo deja una diferencia menor a un peso.
  VERIFICAR(std::llabs(p.diferenciaAlFinal) <= 100);
}

PRUEBA(meta_sin_tasa_reparte_en_partes_iguales) {
  Meta m;
  m.montoObjetivo = 120000;  // $1,200.00
  m.fechaObjetivo = Fecha{2027, 1, 1};
  m.sobreAsociado = 1;
  m.periodicidad = Periodicidad::Mensual;
  m.tasaAnual = 0.0;
  m.aportePlaneado = 10000;

  const ProyeccionMeta p = proyectarMeta(m, 0, Fecha{2026, 1, 1});
  VERIFICAR_IGUAL(p.periodos, 12LL);
  VERIFICAR_IGUAL(p.aporteRequerido, static_cast<Centavos>(10000));
  VERIFICAR(p.estado == EstadoMeta::EnLinea);
  VERIFICAR_IGUAL(p.valorFuturoProyectado, static_cast<Centavos>(120000));
}

PRUEBA(meta_ya_alcanzada_no_pide_aportes) {
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 12000000, kHoy);
  VERIFICAR(p.estado == EstadoMeta::YaAlcanzada);
  VERIFICAR_IGUAL(p.aporteRequerido, static_cast<Centavos>(0));
  VERIFICAR(p.tieneFechaEstimada);
  VERIFICAR(p.fechaEstimada == kHoy);
}

PRUEBA(meta_con_puro_interes_suficiente_no_pide_aportes) {
  Meta m = metaBase();
  m.montoObjetivo = 2100000;  // el saldo solo ya crece más allá del objetivo
  m.aportePlaneado = 0;
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(p.proyectable);
  VERIFICAR_IGUAL(p.aporteRequerido, static_cast<Centavos>(0));
  VERIFICAR(p.estado == EstadoMeta::Adelantado);
}

PRUEBA(meta_sin_periodos_restantes_no_es_proyectable) {
  Meta m = metaBase();
  m.fechaObjetivo = Fecha{2025, 12, 1};  // ya pasó
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(!p.proyectable);
  VERIFICAR(!p.problema.empty());
  // Aun así dice cuánto falta, que es la pregunta que uno se hace.
  VERIFICAR_IGUAL(p.aporteRequerido, static_cast<Centavos>(8000000));
}

PRUEBA(meta_rechaza_datos_imposibles) {
  Meta m = metaBase();
  m.montoObjetivo = 0;
  VERIFICAR(!proyectarMeta(m, 0, kHoy).proyectable);

  m = metaBase();
  m.tasaAnual = -1.5;
  VERIFICAR(!proyectarMeta(m, 0, kHoy).proyectable);

  m = metaBase();
  m.fechaObjetivo = Fecha{2028, 2, 30};
  VERIFICAR(!proyectarMeta(m, 0, kHoy).proyectable);
}

PRUEBA(meta_estima_la_fecha_de_alcance) {
  const Meta m = metaBase();
  const ProyeccionMeta p = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(p.tieneFechaEstimada);
  // Con $3,000.00 mensuales hacen falta 23.75 periodos, es decir 24 aportes.
  VERIFICAR_IGUAL(p.periodosHastaAlcanzar, 24LL);
  VERIFICAR(p.fechaEstimada == (Fecha{2028, 1, 1}));
}

PRUEBA(meta_funciona_con_quincenas_y_semanas) {
  Meta m = metaBase();
  m.periodicidad = Periodicidad::Quincenal;
  const ProyeccionMeta q = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR_IGUAL(q.periodos, 48LL);
  VERIFICAR_CERCA(std::pow(1.0 + q.tasaPorPeriodo, 24) - 1.0, 0.08, 1e-12);

  m.periodicidad = Periodicidad::Semanal;
  const ProyeccionMeta s = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR_IGUAL(s.periodos, 104LL);
  VERIFICAR_CERCA(std::pow(1.0 + s.tasaPorPeriodo, 52) - 1.0, 0.08, 1e-12);
}

PRUEBA(meta_el_resumen_dice_adelantado_o_atrasado) {
  Meta m = metaBase();
  const ProyeccionMeta adelantado = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(resumirProyeccion(adelantado, m.periodicidad)
                .find("adelantado") != std::string::npos);

  m.aportePlaneado = 100000;
  const ProyeccionMeta atrasado = proyectarMeta(m, 2000000, kHoy);
  VERIFICAR(resumirProyeccion(atrasado, m.periodicidad)
                .find("atrasado") != std::string::npos);
}
