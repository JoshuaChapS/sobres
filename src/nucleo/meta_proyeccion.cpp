#include "meta_proyeccion.h"

#include <cmath>

#include "recurrencia.h"

namespace sobres {
namespace {

// Por debajo de esta tasa por periodo se usan las fórmulas sin interés: la
// división entre i perdería precisión antes de aportar algo al resultado.
constexpr double kTasaCasiCero = 1e-12;
// Un peso de tolerancia para decidir si una meta va "en línea".
constexpr Centavos kToleranciaEnLinea = 100;

double factorCrecimiento(double i, long long n) {
  return std::pow(1.0 + i, static_cast<double>(n));
}

}  // namespace

ProyeccionMeta proyectarMeta(const Meta& meta, Centavos saldoActual,
                             const Fecha& hoy) {
  ProyeccionMeta p;
  p.saldoActual = saldoActual;
  p.montoObjetivo = meta.montoObjetivo;
  p.aportePlaneado = meta.aportePlaneado;

  if (meta.montoObjetivo <= 0) {
    p.problema = "La meta no tiene un monto objetivo mayor que cero.";
    return p;
  }
  if (!esFechaValida(meta.fechaObjetivo)) {
    p.problema = "La fecha objetivo no es una fecha válida.";
    return p;
  }
  if (meta.tasaAnual <= -1.0 || !std::isfinite(meta.tasaAnual)) {
    p.problema = "La tasa anual esperada no es un número usable.";
    return p;
  }

  const int m = periodosPorAnio(meta.periodicidad);
  const double i = std::pow(1.0 + meta.tasaAnual, 1.0 / m) - 1.0;
  p.tasaPorPeriodo = i;

  const long long n = periodosEntre(hoy, meta.fechaObjetivo, meta.periodicidad);
  p.periodos = n;

  // Caso simple y frecuente: el sobre ya tiene lo necesario.
  if (saldoActual >= meta.montoObjetivo) {
    p.proyectable = true;
    p.estado = EstadoMeta::YaAlcanzada;
    p.aporteRequerido = 0;
    p.diferenciaPorPeriodo = meta.aportePlaneado;
    p.valorFuturoProyectado = redondearCentavos(
        static_cast<double>(saldoActual) * factorCrecimiento(i, n) +
        (i > kTasaCasiCero
             ? static_cast<double>(meta.aportePlaneado) *
                   (factorCrecimiento(i, n) - 1.0) / i
             : static_cast<double>(meta.aportePlaneado) * n));
    p.diferenciaAlFinal = p.valorFuturoProyectado - meta.montoObjetivo;
    p.tieneFechaEstimada = true;
    p.fechaEstimada = hoy;
    p.periodosHastaAlcanzar = 0;
    return p;
  }

  if (n <= 0) {
    p.problema =
        "Ya no queda ningún periodo completo antes de la fecha objetivo: "
        "faltan " +
        formatearPesos(meta.montoObjetivo - saldoActual) +
        " y habría que ponerlos de una sola vez.";
    p.estado = EstadoMeta::NoProyectable;
    p.aporteRequerido = meta.montoObjetivo - saldoActual;
    return p;
  }

  const double vp = static_cast<double>(saldoActual);
  const double vf = static_cast<double>(meta.montoObjetivo);
  const double factor = factorCrecimiento(i, n);

  // Aporte requerido: se despeja A de la ecuación de la anualidad.
  double aporteRequerido;
  if (i > kTasaCasiCero) {
    aporteRequerido = (vf - vp * factor) * i / (factor - 1.0);
  } else {
    aporteRequerido = (vf - vp) / static_cast<double>(n);
  }
  // Si el puro interés ya rebasa el objetivo, no hace falta aportar nada.
  if (aporteRequerido < 0.0) aporteRequerido = 0.0;
  p.aporteRequerido = redondearCentavos(aporteRequerido);
  p.diferenciaPorPeriodo = meta.aportePlaneado - p.aporteRequerido;

  // Valor futuro al que se llega con el aporte que el usuario planeó.
  const double a = static_cast<double>(meta.aportePlaneado);
  const double proyectado =
      i > kTasaCasiCero ? vp * factor + a * (factor - 1.0) / i
                        : vp + a * static_cast<double>(n);
  p.valorFuturoProyectado = redondearCentavos(proyectado);
  p.diferenciaAlFinal = p.valorFuturoProyectado - meta.montoObjetivo;

  if (p.diferenciaAlFinal > kToleranciaEnLinea) {
    p.estado = EstadoMeta::Adelantado;
  } else if (p.diferenciaAlFinal < -kToleranciaEnLinea) {
    p.estado = EstadoMeta::Atrasado;
  } else {
    p.estado = EstadoMeta::EnLinea;
  }
  p.proyectable = true;

  // Fecha estimada de alcance con el aporte planeado. Se despeja n de la misma
  // ecuación: (1+i)^n = (VF·i + A) / (VP·i + A).
  if (meta.aportePlaneado > 0) {
    double periodosNecesarios = -1.0;
    if (i > kTasaCasiCero) {
      const double numerador = vf * i + a;
      const double denominador = vp * i + a;
      if (denominador > 0.0 && numerador > 0.0 &&
          numerador / denominador > 1.0) {
        periodosNecesarios = std::log(numerador / denominador) /
                             std::log(1.0 + i);
      }
    } else {
      periodosNecesarios = (vf - vp) / a;
    }
    if (periodosNecesarios >= 0.0 && std::isfinite(periodosNecesarios) &&
        periodosNecesarios < 21000.0) {
      const long long k =
          static_cast<long long>(std::ceil(periodosNecesarios - 1e-9));
      p.tieneFechaEstimada = true;
      p.periodosHastaAlcanzar = k;
      p.fechaEstimada = avanzarPeriodos(hoy, meta.periodicidad, k);
    }
  }

  return p;
}

std::string resumirProyeccion(const ProyeccionMeta& p,
                              Periodicidad periodicidad) {
  if (!p.proyectable) {
    return p.problema.empty() ? "No se puede proyectar esta meta." : p.problema;
  }

  const std::string cadaPeriodo =
      periodicidad == Periodicidad::Semanal
          ? "cada semana"
          : (periodicidad == Periodicidad::Quincenal ? "cada quincena"
                                                     : "cada mes");

  switch (p.estado) {
    case EstadoMeta::YaAlcanzada:
      return "La meta ya está cubierta: el sobre tiene " +
             formatearPesos(p.saldoActual) + " y el objetivo era " +
             formatearPesos(p.montoObjetivo) + ".";
    case EstadoMeta::EnLinea:
      return "Vas justo. Aportando " + formatearPesos(p.aportePlaneado) + " " +
             cadaPeriodo + " durante " + std::to_string(p.periodos) +
             " periodos llegas al objetivo casi exacto.";
    case EstadoMeta::Adelantado:
      return "Vas adelantado por " + formatearPesos(p.diferenciaAlFinal) +
             ". Bastaría con aportar " + formatearPesos(p.aporteRequerido) +
             " " + cadaPeriodo + ", " +
             formatearPesos(p.diferenciaPorPeriodo) + " menos de lo que tenías "
             "planeado.";
    case EstadoMeta::Atrasado:
      return "Vas atrasado por " + formatearPesos(-p.diferenciaAlFinal) +
             ". Para llegar a tiempo tendrías que aportar " +
             formatearPesos(p.aporteRequerido) + " " + cadaPeriodo + ", " +
             formatearPesos(-p.diferenciaPorPeriodo) + " más de lo planeado.";
    case EstadoMeta::NoProyectable:
      break;
  }
  return p.problema;
}

}  // namespace sobres
