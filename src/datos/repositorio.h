// Sobres — repositorio.h
// Toda la persistencia en un solo lugar: una base SQLite (Structured Query
// Language Lite, la base de datos de un solo archivo) que vive en el disco del
// usuario. No hay servidor, no hay red y no hay cuentas.
//
// El acceso va por QtSql con el controlador QSQLITE, que Qt trae incluido, así
// que no hace falta instalar SQLite aparte.
#pragma once

#include <QSqlDatabase>
#include <QString>
#include <optional>
#include <vector>

#include "../nucleo/modelos.h"

namespace sobres {

class Repositorio {
 public:
  Repositorio();
  ~Repositorio();

  Repositorio(const Repositorio&) = delete;
  Repositorio& operator=(const Repositorio&) = delete;

  // Abre (o crea) el archivo de datos y deja el esquema al día.
  bool abrir(const QString& rutaArchivo, QString* error);
  void cerrar();
  bool estaAbierto() const;
  QString rutaArchivo() const { return ruta_; }

  // Ruta por omisión: una carpeta propia dentro de los datos de la aplicación
  // del usuario.
  static QString rutaPorOmision();

  // True si la base todavía no tiene ningún sobre: dispara el onboarding.
  bool estaVacia() const;

  // --- Sobres ------------------------------------------------------------
  std::vector<Sobre> sobres(bool incluirArchivados = false) const;
  std::optional<Sobre> sobre(Id id) const;
  bool guardarSobre(Sobre& s, QString* error);       // inserta si id es kSinId
  bool archivarSobre(Id id, bool archivado, QString* error);
  bool eliminarSobre(Id id, QString* error);         // falla si tiene historia
  bool moverSobre(Id id, int desplazamiento, QString* error);  // -1 o +1
  int movimientosDelSobre(Id id) const;

  // --- Categorías --------------------------------------------------------
  std::vector<Categoria> categorias(bool incluirArchivadas = false) const;
  bool guardarCategoria(Categoria& c, QString* error);
  bool archivarCategoria(Id id, bool archivada, QString* error);
  bool eliminarCategoria(Id id, QString* error);

  // --- Movimientos -------------------------------------------------------
  // Sin filtros devuelve todo, del más reciente al más antiguo.
  std::vector<Movimiento> movimientos() const;
  std::vector<Movimiento> movimientosEntre(const Fecha& desde,
                                           const Fecha& hasta) const;
  bool guardarMovimiento(Movimiento& m, QString* error);
  bool eliminarMovimiento(Id id, QString* error);
  // Guarda varios movimientos como una sola operación: o entran todos o no
  // entra ninguno. Es lo que usan el reparto y los recurrentes.
  bool guardarMovimientos(std::vector<Movimiento>& lista, QString* error);

  // --- Pasivos -----------------------------------------------------------
  std::vector<Pasivo> pasivos() const;
  bool guardarPasivo(Pasivo& p, QString* error);
  bool eliminarPasivo(Id id, QString* error);

  // --- Metas -------------------------------------------------------------
  std::vector<Meta> metas() const;
  bool guardarMeta(Meta& m, QString* error);
  bool eliminarMeta(Id id, QString* error);

  // --- Recurrentes -------------------------------------------------------
  std::vector<Recurrente> recurrentes() const;
  bool guardarRecurrente(Recurrente& r, QString* error);
  bool eliminarRecurrente(Id id, QString* error);
  // Crea el movimiento del recurrente y adelanta su próxima fecha un periodo.
  bool materializarRecurrente(Id id, const Fecha& fecha, QString* error);

  // --- Plantillas de reparto ---------------------------------------------
  std::vector<PlantillaReparto> plantillas() const;
  bool guardarPlantilla(PlantillaReparto& p, QString* error);
  bool eliminarPlantilla(Id id, QString* error);

 private:
  bool crearEsquema(QString* error);
  int versionDeEsquema() const;
  bool fijarVersionDeEsquema(int version, QString* error);
  std::vector<LineaReparto> lineasDePlantilla(Id plantilla) const;

  QSqlDatabase base_;
  QString ruta_;
  QString nombreConexion_;
};

}  // namespace sobres
