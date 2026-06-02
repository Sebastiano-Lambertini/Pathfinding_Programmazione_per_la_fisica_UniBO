#ifndef PATH_H
#define PATH_H

#include <cmath>
#include <vector>

namespace pf {
enum senso_di_percorrenza { orario, antiorario };
struct Point {
  int x;
  int y;

  Point() : x(0), y(0) {}
  Point(int _x, int _y) : x(_x), y(_y) {}

  bool operator==(const Point &p) const { return x == p.x && y == p.y; }

  bool operator<(const Point &p) const {
    if (x != p.x)
      return x < p.x;
    return y < p.y;
  }

  bool e_valido() const { return x >= 0 && y >= 0; }
};

struct Path {
  std::vector<Point> points;
  double cost;
  std::vector<std::pair<int, int>> detours;

  Path() : cost(0) {}
};

struct Border_exit {
  Point exit;
  Path bordo;
  senso_di_percorrenza direzione;
};

class Grid {
private:
  const std::vector<std::vector<bool>> &griglia;
  int larghezza;
  int altezza;
  std::vector<std::vector<int>> id_ostacolo;

public:
  Grid(const std::vector<std::vector<bool>> &griglia);

  bool is_free(int x, int y) const;
  bool is_free(const Point &p) const;
  int get_larghezza() const;
  int get_altezza() const;
  void controlla_id_ostacolo();
  int get_id_ostacolo(int x, int y) const;
  int get_id_ostacolo(const Point &p) const;

private:
  void riempi(int start_x, int start_y, int id);
};

// funzioni
std::vector<Point> linea_dritta(const Point &da, const Point &a);
Point primo_ostacolo_sulla_linea(const Point &A, const Point &B,
                                 const Grid &griglia);
Point ultimo_punto_prima_del_primo_ostacolo(const Point &A, const Point &B,
                                            const Grid &griglia);
bool vede_punto_di_arrivo(const Point &P, const Point &B, const Grid &griglia);
bool vede_spazio_in_direzione_arrivo(const Point &P, const Point &B,
                                     const Grid &griglia, int id_ostacolo);

Border_exit segui_il_bordo_monodirezione(const Point &partenza,
                                         const Point &destinazione,
                                         const Grid &griglia, int id_ostacolo,
                                         senso_di_percorrenza direzione);

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d);

double costo_path_geometrico(const std::vector<Point> &path);

std::vector<std::vector<Point>>
pulisci_all_indietro(const std::vector<Point> &path, const Grid &griglia);

std::vector<std::vector<Point>>
pulisci_fino_a_stabile(const std::vector<Point> &path, const Grid &griglia);

void costruisci_percorsi(const Point &corrente, const Point &destinazione,
                         const Grid &griglia,
                         std::vector<Point> &percorso_parziale,
                         double costo_parziale,
                         std::vector<Path> &percorsi_output, int profondita);

std::vector<Path> trova_paths(const Point &A, const Point &B,
                              const Grid &griglia);

std::vector<Path> filtra_paths(std::vector<Path> &paths);

void printa_path(const Path &p, int width, int height,
                 const std::vector<std::vector<bool>> &freeMap);

std::vector<Path> // funzione wrapper
final_aggiratore(const Point &A, const Point &B,
                 const std::vector<std::vector<bool>> &griglia);
} // namespace pf
#endif