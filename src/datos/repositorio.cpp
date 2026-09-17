#include "repositorio.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

#include "../nucleo/recurrencia.h"

namespace sobres {
namespace {

constexpr int kVersionEsquema = 1;

QString texto(const std::string& s) { return QString::fromStdString(s); }
std::string texto(const QVariant& v) { return v.toString().toStdString(); }

Fecha leerFecha(const QVariant& v) {
  const auto f = desdeTextoIso(v.toString().toStdString());
  return f.value_or(Fecha{});
}

QVariant idOpcional(Id id) {
  // Los identificadores ausentes se guardan como NULL, no como 0, para que las
  // llaves foráneas de SQLite los acepten.
  if (id == kSinId) return QVariant(QMetaType(QMetaType::LongLong));
  return QVariant(static_cast<qlonglong>(id));
}

Id leerId(const QVariant& v) {
  if (v.isNull()) return kSinId;
  return static_cast<Id>(v.toLongLong());
}

bool ejecutar(QSqlQuery& consulta, QString* error) {
  if (consulta.exec()) return true;
  if (error) *error = consulta.lastError().text();
  return false;
}

bool ejecutarTexto(QSqlDatabase& base, const QString& sql, QString* error) {
  QSqlQuery consulta(base);
  if (consulta.exec(sql)) return true;
  if (error) *error = consulta.lastError().text();
  return false;
}

}  // namespace

Repositorio::Repositorio() {
  // Cada repositorio usa su propia conexión con nombre, para que abrir dos
  // archivos a la vez (por ejemplo en las pruebas) no se estorbe.
  nombreConexion_ =
      QStringLiteral("sobres-%1").arg(reinterpret_cast<quintptr>(this));
}

Repositorio::~Repositorio() { cerrar(); }

QString Repositorio::rutaPorOmision() {
  const QString carpeta =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  return QDir(carpeta).filePath(QStringLiteral("sobres.db"));
}

bool Repositorio::abrir(const QString& rutaArchivo, QString* error) {
  cerrar();

  const QFileInfo info(rutaArchivo);
  QDir carpeta = info.absoluteDir();
  if (!carpeta.exists() && !carpeta.mkpath(QStringLiteral("."))) {
    if (error) {
      *error = QStringLiteral("No se pudo crear la carpeta %1")
                   .arg(carpeta.absolutePath());
    }
    return false;
  }

  base_ = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), nombreConexion_);
  base_.setDatabaseName(rutaArchivo);
  if (!base_.open()) {
    if (error) *error = base_.lastError().text();
    QSqlDatabase::removeDatabase(nombreConexion_);
    return false;
  }
  ruta_ = rutaArchivo;

  // Las llaves foráneas de SQLite vienen apagadas por omisión en cada conexión.
  if (!ejecutarTexto(base_, QStringLiteral("PRAGMA foreign_keys = ON"), error)) {
    return false;
  }
  return crearEsquema(error);
}

void Repositorio::cerrar() {
  if (base_.isOpen()) base_.close();
  base_ = QSqlDatabase();
  if (QSqlDatabase::contains(nombreConexion_)) {
    QSqlDatabase::removeDatabase(nombreConexion_);
  }
  ruta_.clear();
}

bool Repositorio::estaAbierto() const { return base_.isOpen(); }

int Repositorio::versionDeEsquema() const {
  QSqlQuery consulta(base_);
  consulta.prepare(
      QStringLiteral("SELECT valor FROM meta_app WHERE clave = 'version'"));
  if (!consulta.exec() || !consulta.next()) return 0;
  return consulta.value(0).toInt();
}

bool Repositorio::fijarVersionDeEsquema(int version, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral(
      "INSERT INTO meta_app (clave, valor) VALUES ('version', ?) "
      "ON CONFLICT(clave) DO UPDATE SET valor = excluded.valor"));
  consulta.addBindValue(QString::number(version));
  return ejecutar(consulta, error);
}

bool Repositorio::crearEsquema(QString* error) {
  static const char* kTablas[] = {
      "CREATE TABLE IF NOT EXISTS meta_app ("
      "  clave TEXT PRIMARY KEY,"
      "  valor TEXT NOT NULL)",

      "CREATE TABLE IF NOT EXISTS sobre ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  color TEXT NOT NULL,"
      "  tipo INTEGER NOT NULL,"
      "  orden INTEGER NOT NULL,"
      "  archivado INTEGER NOT NULL DEFAULT 0)",

      "CREATE TABLE IF NOT EXISTS categoria ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  color TEXT NOT NULL,"
      "  icono TEXT NOT NULL DEFAULT '',"
      "  archivada INTEGER NOT NULL DEFAULT 0)",

      "CREATE TABLE IF NOT EXISTS movimiento ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  fecha TEXT NOT NULL,"
      "  tipo INTEGER NOT NULL,"
      "  sobre_origen INTEGER NOT NULL REFERENCES sobre(id),"
      "  sobre_destino INTEGER REFERENCES sobre(id),"
      "  categoria INTEGER REFERENCES categoria(id) ON DELETE SET NULL,"
      "  monto INTEGER NOT NULL,"
      "  nota TEXT NOT NULL DEFAULT '',"
      "  origen_automatico TEXT NOT NULL DEFAULT '')",

      "CREATE INDEX IF NOT EXISTS idx_movimiento_fecha ON movimiento(fecha)",
      "CREATE INDEX IF NOT EXISTS idx_movimiento_sobre "
      "  ON movimiento(sobre_origen)",

      "CREATE TABLE IF NOT EXISTS pasivo ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  saldo INTEGER NOT NULL,"
      "  nota TEXT NOT NULL DEFAULT '')",

      "CREATE TABLE IF NOT EXISTS meta ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  monto_objetivo INTEGER NOT NULL,"
      "  fecha_objetivo TEXT NOT NULL,"
      "  sobre_asociado INTEGER NOT NULL REFERENCES sobre(id) ON DELETE CASCADE,"
      "  aporte_planeado INTEGER NOT NULL DEFAULT 0,"
      "  periodicidad INTEGER NOT NULL DEFAULT 2,"
      "  tasa_anual REAL NOT NULL DEFAULT 0,"
      "  nota TEXT NOT NULL DEFAULT '')",

      "CREATE TABLE IF NOT EXISTS recurrente ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  tipo INTEGER NOT NULL,"
      "  sobre_origen INTEGER NOT NULL REFERENCES sobre(id) ON DELETE CASCADE,"
      "  sobre_destino INTEGER REFERENCES sobre(id) ON DELETE CASCADE,"
      "  categoria INTEGER REFERENCES categoria(id) ON DELETE SET NULL,"
      "  monto INTEGER NOT NULL,"
      "  nota TEXT NOT NULL DEFAULT '',"
      "  periodicidad INTEGER NOT NULL,"
      "  proxima_fecha TEXT NOT NULL,"
      "  activo INTEGER NOT NULL DEFAULT 1)",

      "CREATE TABLE IF NOT EXISTS plantilla_reparto ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  nombre TEXT NOT NULL,"
      "  modo INTEGER NOT NULL,"
      "  monto_sugerido INTEGER NOT NULL DEFAULT 0,"
      "  sobre_fuente INTEGER REFERENCES sobre(id) ON DELETE SET NULL,"
      "  categoria INTEGER REFERENCES categoria(id) ON DELETE SET NULL)",

      "CREATE TABLE IF NOT EXISTS linea_reparto ("
      "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "  plantilla INTEGER NOT NULL REFERENCES plantilla_reparto(id)"
      "    ON DELETE CASCADE,"
      "  sobre INTEGER NOT NULL REFERENCES sobre(id) ON DELETE CASCADE,"
      "  valor INTEGER NOT NULL,"
      "  recibe_resto INTEGER NOT NULL DEFAULT 0,"
      "  orden INTEGER NOT NULL DEFAULT 0)",
  };

  for (const char* sql : kTablas) {
    if (!ejecutarTexto(base_, QString::fromLatin1(sql), error)) return false;
  }

  const int version = versionDeEsquema();
  if (version == 0) {
    return fijarVersionDeEsquema(kVersionEsquema, error);
  }
  if (version > kVersionEsquema) {
    if (error) {
      *error = QStringLiteral(
                   "El archivo de datos fue creado por una versión más nueva "
                   "de Sobres (esquema %1).")
                   .arg(version);
    }
    return false;
  }
  // Aquí irían las migraciones cuando kVersionEsquema suba.
  return true;
}

bool Repositorio::estaVacia() const {
  QSqlQuery consulta(base_);
  if (!consulta.exec(QStringLiteral("SELECT COUNT(*) FROM sobre"))) return true;
  if (!consulta.next()) return true;
  return consulta.value(0).toInt() == 0;
}

// ---------------------------------------------------------------------------
// Sobres
// ---------------------------------------------------------------------------

std::vector<Sobre> Repositorio::sobres(bool incluirArchivados) const {
  std::vector<Sobre> lista;
  QSqlQuery consulta(base_);
  const QString sql =
      QStringLiteral("SELECT id, nombre, color, tipo, orden, archivado "
                     "FROM sobre %1 ORDER BY orden, id")
          .arg(incluirArchivados ? QString() : QStringLiteral("WHERE archivado = 0"));
  if (!consulta.exec(sql)) return lista;
  while (consulta.next()) {
    Sobre s;
    s.id = consulta.value(0).toLongLong();
    s.nombre = texto(consulta.value(1));
    s.color = texto(consulta.value(2));
    s.tipo = static_cast<TipoSobre>(consulta.value(3).toInt());
    s.orden = consulta.value(4).toInt();
    s.archivado = consulta.value(5).toInt() != 0;
    lista.push_back(s);
  }
  return lista;
}

std::optional<Sobre> Repositorio::sobre(Id id) const {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral(
      "SELECT id, nombre, color, tipo, orden, archivado FROM sobre WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  if (!consulta.exec() || !consulta.next()) return std::nullopt;
  Sobre s;
  s.id = consulta.value(0).toLongLong();
  s.nombre = texto(consulta.value(1));
  s.color = texto(consulta.value(2));
  s.tipo = static_cast<TipoSobre>(consulta.value(3).toInt());
  s.orden = consulta.value(4).toInt();
  s.archivado = consulta.value(5).toInt() != 0;
  return s;
}

bool Repositorio::guardarSobre(Sobre& s, QString* error) {
  QSqlQuery consulta(base_);
  if (s.id == kSinId) {
    // Un sobre nuevo se va al final de la lista.
    QSqlQuery maximo(base_);
    int siguienteOrden = 0;
    if (maximo.exec(QStringLiteral("SELECT COALESCE(MAX(orden), -1) FROM sobre")) &&
        maximo.next()) {
      siguienteOrden = maximo.value(0).toInt() + 1;
    }
    s.orden = siguienteOrden;
    consulta.prepare(QStringLiteral(
        "INSERT INTO sobre (nombre, color, tipo, orden, archivado) "
        "VALUES (?, ?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE sobre SET nombre = ?, color = ?, tipo = ?, orden = ?, "
        "archivado = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(s.nombre));
  consulta.addBindValue(texto(s.color));
  consulta.addBindValue(static_cast<int>(s.tipo));
  consulta.addBindValue(s.orden);
  consulta.addBindValue(s.archivado ? 1 : 0);
  if (s.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(s.id));

  if (!ejecutar(consulta, error)) return false;
  if (s.id == kSinId) s.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::archivarSobre(Id id, bool archivado, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("UPDATE sobre SET archivado = ? WHERE id = ?"));
  consulta.addBindValue(archivado ? 1 : 0);
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

int Repositorio::movimientosDelSobre(Id id) const {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral(
      "SELECT COUNT(*) FROM movimiento "
      "WHERE sobre_origen = ? OR sobre_destino = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  consulta.addBindValue(static_cast<qlonglong>(id));
  if (!consulta.exec() || !consulta.next()) return 0;
  return consulta.value(0).toInt();
}

bool Repositorio::eliminarSobre(Id id, QString* error) {
  if (movimientosDelSobre(id) > 0) {
    if (error) {
      *error = QStringLiteral(
          "Este sobre tiene movimientos registrados. Archívalo en lugar de "
          "borrarlo, así el historial se conserva.");
    }
    return false;
  }
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM sobre WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

bool Repositorio::moverSobre(Id id, int desplazamiento, QString* error) {
  if (desplazamiento == 0) return true;
  std::vector<Sobre> lista = sobres(true);
  std::size_t posicion = lista.size();
  for (std::size_t k = 0; k < lista.size(); ++k) {
    if (lista[k].id == id) posicion = k;
  }
  if (posicion == lista.size()) {
    if (error) *error = QStringLiteral("El sobre ya no existe.");
    return false;
  }
  const long long destino =
      static_cast<long long>(posicion) + desplazamiento;
  if (destino < 0 || destino >= static_cast<long long>(lista.size())) {
    return true;  // ya está en el extremo, no es un error
  }
  std::swap(lista[posicion], lista[static_cast<std::size_t>(destino)]);

  if (!base_.transaction()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  for (std::size_t k = 0; k < lista.size(); ++k) {
    QSqlQuery consulta(base_);
    consulta.prepare(QStringLiteral("UPDATE sobre SET orden = ? WHERE id = ?"));
    consulta.addBindValue(static_cast<int>(k));
    consulta.addBindValue(static_cast<qlonglong>(lista[k].id));
    if (!ejecutar(consulta, error)) {
      base_.rollback();
      return false;
    }
  }
  if (!base_.commit()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Categorías
// ---------------------------------------------------------------------------

std::vector<Categoria> Repositorio::categorias(bool incluirArchivadas) const {
  std::vector<Categoria> lista;
  QSqlQuery consulta(base_);
  const QString sql =
      QStringLiteral("SELECT id, nombre, color, icono, archivada FROM categoria "
                     "%1 ORDER BY nombre COLLATE NOCASE")
          .arg(incluirArchivadas ? QString()
                                 : QStringLiteral("WHERE archivada = 0"));
  if (!consulta.exec(sql)) return lista;
  while (consulta.next()) {
    Categoria c;
    c.id = consulta.value(0).toLongLong();
    c.nombre = texto(consulta.value(1));
    c.color = texto(consulta.value(2));
    c.icono = texto(consulta.value(3));
    c.archivada = consulta.value(4).toInt() != 0;
    lista.push_back(c);
  }
  return lista;
}

bool Repositorio::guardarCategoria(Categoria& c, QString* error) {
  QSqlQuery consulta(base_);
  if (c.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO categoria (nombre, color, icono, archivada) "
        "VALUES (?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE categoria SET nombre = ?, color = ?, icono = ?, archivada = ? "
        "WHERE id = ?"));
  }
  consulta.addBindValue(texto(c.nombre));
  consulta.addBindValue(texto(c.color));
  consulta.addBindValue(texto(c.icono));
  consulta.addBindValue(c.archivada ? 1 : 0);
  if (c.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(c.id));

  if (!ejecutar(consulta, error)) return false;
  if (c.id == kSinId) c.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::archivarCategoria(Id id, bool archivada, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(
      QStringLiteral("UPDATE categoria SET archivada = ? WHERE id = ?"));
  consulta.addBindValue(archivada ? 1 : 0);
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

bool Repositorio::eliminarCategoria(Id id, QString* error) {
  // Los movimientos que la usaban quedan sin categoría, no se borran.
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM categoria WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

// ---------------------------------------------------------------------------
// Movimientos
// ---------------------------------------------------------------------------

namespace {

const char* kSelectMovimiento =
    "SELECT id, fecha, tipo, sobre_origen, sobre_destino, categoria, monto, "
    "nota, origen_automatico FROM movimiento ";

Movimiento leerMovimiento(const QSqlQuery& consulta) {
  Movimiento m;
  m.id = consulta.value(0).toLongLong();
  m.fecha = leerFecha(consulta.value(1));
  m.tipo = static_cast<TipoMovimiento>(consulta.value(2).toInt());
  m.sobreOrigen = leerId(consulta.value(3));
  m.sobreDestino = leerId(consulta.value(4));
  m.categoria = leerId(consulta.value(5));
  m.monto = consulta.value(6).toLongLong();
  m.nota = texto(consulta.value(7));
  m.origenAutomatico = texto(consulta.value(8));
  return m;
}

}  // namespace

std::vector<Movimiento> Repositorio::movimientos() const {
  std::vector<Movimiento> lista;
  QSqlQuery consulta(base_);
  if (!consulta.exec(QString::fromLatin1(kSelectMovimiento) +
                     QStringLiteral("ORDER BY fecha DESC, id DESC"))) {
    return lista;
  }
  while (consulta.next()) lista.push_back(leerMovimiento(consulta));
  return lista;
}

std::vector<Movimiento> Repositorio::movimientosEntre(const Fecha& desde,
                                                      const Fecha& hasta) const {
  std::vector<Movimiento> lista;
  QSqlQuery consulta(base_);
  consulta.prepare(QString::fromLatin1(kSelectMovimiento) +
                   QStringLiteral("WHERE fecha >= ? AND fecha <= ? "
                                  "ORDER BY fecha DESC, id DESC"));
  consulta.addBindValue(texto(aTextoIso(desde)));
  consulta.addBindValue(texto(aTextoIso(hasta)));
  if (!consulta.exec()) return lista;
  while (consulta.next()) lista.push_back(leerMovimiento(consulta));
  return lista;
}

bool Repositorio::guardarMovimiento(Movimiento& m, QString* error) {
  QSqlQuery consulta(base_);
  if (m.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO movimiento (fecha, tipo, sobre_origen, sobre_destino, "
        "categoria, monto, nota, origen_automatico) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE movimiento SET fecha = ?, tipo = ?, sobre_origen = ?, "
        "sobre_destino = ?, categoria = ?, monto = ?, nota = ?, "
        "origen_automatico = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(aTextoIso(m.fecha)));
  consulta.addBindValue(static_cast<int>(m.tipo));
  consulta.addBindValue(static_cast<qlonglong>(m.sobreOrigen));
  // El sobre de destino solo tiene sentido en un traspaso.
  consulta.addBindValue(m.tipo == TipoMovimiento::Traspaso
                            ? idOpcional(m.sobreDestino)
                            : idOpcional(kSinId));
  consulta.addBindValue(idOpcional(m.categoria));
  consulta.addBindValue(static_cast<qlonglong>(m.monto));
  consulta.addBindValue(texto(m.nota));
  consulta.addBindValue(texto(m.origenAutomatico));
  if (m.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(m.id));

  if (!ejecutar(consulta, error)) return false;
  if (m.id == kSinId) m.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::guardarMovimientos(std::vector<Movimiento>& lista,
                                     QString* error) {
  if (lista.empty()) return true;
  if (!base_.transaction()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  for (Movimiento& m : lista) {
    if (!guardarMovimiento(m, error)) {
      base_.rollback();
      return false;
    }
  }
  if (!base_.commit()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  return true;
}

bool Repositorio::eliminarMovimiento(Id id, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM movimiento WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

// ---------------------------------------------------------------------------
// Pasivos
// ---------------------------------------------------------------------------

std::vector<Pasivo> Repositorio::pasivos() const {
  std::vector<Pasivo> lista;
  QSqlQuery consulta(base_);
  if (!consulta.exec(QStringLiteral(
          "SELECT id, nombre, saldo, nota FROM pasivo ORDER BY id"))) {
    return lista;
  }
  while (consulta.next()) {
    Pasivo p;
    p.id = consulta.value(0).toLongLong();
    p.nombre = texto(consulta.value(1));
    p.saldo = consulta.value(2).toLongLong();
    p.nota = texto(consulta.value(3));
    lista.push_back(p);
  }
  return lista;
}

bool Repositorio::guardarPasivo(Pasivo& p, QString* error) {
  QSqlQuery consulta(base_);
  if (p.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO pasivo (nombre, saldo, nota) VALUES (?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE pasivo SET nombre = ?, saldo = ?, nota = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(p.nombre));
  consulta.addBindValue(static_cast<qlonglong>(p.saldo));
  consulta.addBindValue(texto(p.nota));
  if (p.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(p.id));

  if (!ejecutar(consulta, error)) return false;
  if (p.id == kSinId) p.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::eliminarPasivo(Id id, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM pasivo WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

// ---------------------------------------------------------------------------
// Metas
// ---------------------------------------------------------------------------

std::vector<Meta> Repositorio::metas() const {
  std::vector<Meta> lista;
  QSqlQuery consulta(base_);
  if (!consulta.exec(QStringLiteral(
          "SELECT id, nombre, monto_objetivo, fecha_objetivo, sobre_asociado, "
          "aporte_planeado, periodicidad, tasa_anual, nota FROM meta "
          "ORDER BY fecha_objetivo, id"))) {
    return lista;
  }
  while (consulta.next()) {
    Meta m;
    m.id = consulta.value(0).toLongLong();
    m.nombre = texto(consulta.value(1));
    m.montoObjetivo = consulta.value(2).toLongLong();
    m.fechaObjetivo = leerFecha(consulta.value(3));
    m.sobreAsociado = leerId(consulta.value(4));
    m.aportePlaneado = consulta.value(5).toLongLong();
    m.periodicidad = static_cast<Periodicidad>(consulta.value(6).toInt());
    m.tasaAnual = consulta.value(7).toDouble();
    m.nota = texto(consulta.value(8));
    lista.push_back(m);
  }
  return lista;
}

bool Repositorio::guardarMeta(Meta& m, QString* error) {
  QSqlQuery consulta(base_);
  if (m.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO meta (nombre, monto_objetivo, fecha_objetivo, "
        "sobre_asociado, aporte_planeado, periodicidad, tasa_anual, nota) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE meta SET nombre = ?, monto_objetivo = ?, fecha_objetivo = ?, "
        "sobre_asociado = ?, aporte_planeado = ?, periodicidad = ?, "
        "tasa_anual = ?, nota = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(m.nombre));
  consulta.addBindValue(static_cast<qlonglong>(m.montoObjetivo));
  consulta.addBindValue(texto(aTextoIso(m.fechaObjetivo)));
  consulta.addBindValue(static_cast<qlonglong>(m.sobreAsociado));
  consulta.addBindValue(static_cast<qlonglong>(m.aportePlaneado));
  consulta.addBindValue(static_cast<int>(m.periodicidad));
  consulta.addBindValue(m.tasaAnual);
  consulta.addBindValue(texto(m.nota));
  if (m.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(m.id));

  if (!ejecutar(consulta, error)) return false;
  if (m.id == kSinId) m.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::eliminarMeta(Id id, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM meta WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

// ---------------------------------------------------------------------------
// Recurrentes
// ---------------------------------------------------------------------------

std::vector<Recurrente> Repositorio::recurrentes() const {
  std::vector<Recurrente> lista;
  QSqlQuery consulta(base_);
  if (!consulta.exec(QStringLiteral(
          "SELECT id, nombre, tipo, sobre_origen, sobre_destino, categoria, "
          "monto, nota, periodicidad, proxima_fecha, activo FROM recurrente "
          "ORDER BY activo DESC, proxima_fecha, id"))) {
    return lista;
  }
  while (consulta.next()) {
    Recurrente r;
    r.id = consulta.value(0).toLongLong();
    r.nombre = texto(consulta.value(1));
    r.tipo = static_cast<TipoMovimiento>(consulta.value(2).toInt());
    r.sobreOrigen = leerId(consulta.value(3));
    r.sobreDestino = leerId(consulta.value(4));
    r.categoria = leerId(consulta.value(5));
    r.monto = consulta.value(6).toLongLong();
    r.nota = texto(consulta.value(7));
    r.periodicidad = static_cast<Periodicidad>(consulta.value(8).toInt());
    r.proximaFecha = leerFecha(consulta.value(9));
    r.activo = consulta.value(10).toInt() != 0;
    lista.push_back(r);
  }
  return lista;
}

bool Repositorio::guardarRecurrente(Recurrente& r, QString* error) {
  QSqlQuery consulta(base_);
  if (r.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO recurrente (nombre, tipo, sobre_origen, sobre_destino, "
        "categoria, monto, nota, periodicidad, proxima_fecha, activo) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE recurrente SET nombre = ?, tipo = ?, sobre_origen = ?, "
        "sobre_destino = ?, categoria = ?, monto = ?, nota = ?, "
        "periodicidad = ?, proxima_fecha = ?, activo = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(r.nombre));
  consulta.addBindValue(static_cast<int>(r.tipo));
  consulta.addBindValue(static_cast<qlonglong>(r.sobreOrigen));
  consulta.addBindValue(r.tipo == TipoMovimiento::Traspaso
                            ? idOpcional(r.sobreDestino)
                            : idOpcional(kSinId));
  consulta.addBindValue(idOpcional(r.categoria));
  consulta.addBindValue(static_cast<qlonglong>(r.monto));
  consulta.addBindValue(texto(r.nota));
  consulta.addBindValue(static_cast<int>(r.periodicidad));
  consulta.addBindValue(texto(aTextoIso(r.proximaFecha)));
  consulta.addBindValue(r.activo ? 1 : 0);
  if (r.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(r.id));

  if (!ejecutar(consulta, error)) return false;
  if (r.id == kSinId) r.id = consulta.lastInsertId().toLongLong();
  return true;
}

bool Repositorio::eliminarRecurrente(Id id, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM recurrente WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

bool Repositorio::materializarRecurrente(Id id, const Fecha& fecha,
                                         QString* error) {
  Recurrente encontrado;
  bool hallado = false;
  for (const Recurrente& r : recurrentes()) {
    if (r.id == id) {
      encontrado = r;
      hallado = true;
      break;
    }
  }
  if (!hallado) {
    if (error) *error = QStringLiteral("El recurrente ya no existe.");
    return false;
  }

  Movimiento m;
  m.fecha = fecha;
  m.tipo = encontrado.tipo;
  m.sobreOrigen = encontrado.sobreOrigen;
  m.sobreDestino = encontrado.sobreDestino;
  m.categoria = encontrado.categoria;
  m.monto = encontrado.monto;
  m.nota = encontrado.nota.empty() ? encontrado.nombre : encontrado.nota;
  m.origenAutomatico = "recurrente:" + encontrado.nombre;

  if (!base_.transaction()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  if (!guardarMovimiento(m, error)) {
    base_.rollback();
    return false;
  }
  // La próxima fecha avanza un periodo desde la que estaba programada, no desde
  // hoy: así un recurrente atrasado se pone al día apretando el botón varias
  // veces sin perder ninguna ocurrencia.
  encontrado.proximaFecha =
      siguienteFecha(encontrado.proximaFecha, encontrado.periodicidad);
  if (!guardarRecurrente(encontrado, error)) {
    base_.rollback();
    return false;
  }
  if (!base_.commit()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  return true;
}

// ---------------------------------------------------------------------------
// Plantillas de reparto
// ---------------------------------------------------------------------------

std::vector<LineaReparto> Repositorio::lineasDePlantilla(Id plantilla) const {
  std::vector<LineaReparto> lineas;
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral(
      "SELECT sobre, valor, recibe_resto FROM linea_reparto "
      "WHERE plantilla = ? ORDER BY orden, id"));
  consulta.addBindValue(static_cast<qlonglong>(plantilla));
  if (!consulta.exec()) return lineas;
  while (consulta.next()) {
    LineaReparto l;
    l.sobre = consulta.value(0).toLongLong();
    l.valor = consulta.value(1).toLongLong();
    l.recibeResto = consulta.value(2).toInt() != 0;
    lineas.push_back(l);
  }
  return lineas;
}

std::vector<PlantillaReparto> Repositorio::plantillas() const {
  std::vector<PlantillaReparto> lista;
  QSqlQuery consulta(base_);
  if (!consulta.exec(QStringLiteral(
          "SELECT id, nombre, modo, monto_sugerido, sobre_fuente, categoria "
          "FROM plantilla_reparto ORDER BY nombre COLLATE NOCASE"))) {
    return lista;
  }
  while (consulta.next()) {
    PlantillaReparto p;
    p.id = consulta.value(0).toLongLong();
    p.nombre = texto(consulta.value(1));
    p.modo = static_cast<ModoReparto>(consulta.value(2).toInt());
    p.montoSugerido = consulta.value(3).toLongLong();
    p.sobreFuente = leerId(consulta.value(4));
    p.categoria = leerId(consulta.value(5));
    lista.push_back(p);
  }
  for (PlantillaReparto& p : lista) p.lineas = lineasDePlantilla(p.id);
  return lista;
}

bool Repositorio::guardarPlantilla(PlantillaReparto& p, QString* error) {
  if (!base_.transaction()) {
    if (error) *error = base_.lastError().text();
    return false;
  }

  QSqlQuery consulta(base_);
  if (p.id == kSinId) {
    consulta.prepare(QStringLiteral(
        "INSERT INTO plantilla_reparto (nombre, modo, monto_sugerido, "
        "sobre_fuente, categoria) VALUES (?, ?, ?, ?, ?)"));
  } else {
    consulta.prepare(QStringLiteral(
        "UPDATE plantilla_reparto SET nombre = ?, modo = ?, "
        "monto_sugerido = ?, sobre_fuente = ?, categoria = ? WHERE id = ?"));
  }
  consulta.addBindValue(texto(p.nombre));
  consulta.addBindValue(static_cast<int>(p.modo));
  consulta.addBindValue(static_cast<qlonglong>(p.montoSugerido));
  consulta.addBindValue(idOpcional(p.sobreFuente));
  consulta.addBindValue(idOpcional(p.categoria));
  if (p.id != kSinId) consulta.addBindValue(static_cast<qlonglong>(p.id));

  if (!ejecutar(consulta, error)) {
    base_.rollback();
    return false;
  }
  if (p.id == kSinId) p.id = consulta.lastInsertId().toLongLong();

  // Las líneas se reescriben completas: son pocas y así no hay que llevar
  // cuenta de cuáles se editaron, cuáles se agregaron y cuáles se quitaron.
  QSqlQuery borrado(base_);
  borrado.prepare(
      QStringLiteral("DELETE FROM linea_reparto WHERE plantilla = ?"));
  borrado.addBindValue(static_cast<qlonglong>(p.id));
  if (!ejecutar(borrado, error)) {
    base_.rollback();
    return false;
  }

  for (std::size_t k = 0; k < p.lineas.size(); ++k) {
    QSqlQuery alta(base_);
    alta.prepare(QStringLiteral(
        "INSERT INTO linea_reparto (plantilla, sobre, valor, recibe_resto, "
        "orden) VALUES (?, ?, ?, ?, ?)"));
    alta.addBindValue(static_cast<qlonglong>(p.id));
    alta.addBindValue(static_cast<qlonglong>(p.lineas[k].sobre));
    alta.addBindValue(static_cast<qlonglong>(p.lineas[k].valor));
    alta.addBindValue(p.lineas[k].recibeResto ? 1 : 0);
    alta.addBindValue(static_cast<int>(k));
    if (!ejecutar(alta, error)) {
      base_.rollback();
      return false;
    }
  }

  if (!base_.commit()) {
    if (error) *error = base_.lastError().text();
    return false;
  }
  return true;
}

bool Repositorio::eliminarPlantilla(Id id, QString* error) {
  QSqlQuery consulta(base_);
  consulta.prepare(QStringLiteral("DELETE FROM plantilla_reparto WHERE id = ?"));
  consulta.addBindValue(static_cast<qlonglong>(id));
  return ejecutar(consulta, error);
}

}  // namespace sobres
