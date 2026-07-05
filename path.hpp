#ifndef PATH_H
#define PATH_H

#include <cmath>
#include <vector>

namespace pf {
enum senso_di_percorrenza { orario, antiorario };
enum class TipoCella {
  oltrepassabile,
  non_oltrepassabile
};
struct Punto {
  int x;
  int y;

  Punto() : x(0), y(0) {}
  Punto(int _x, int _y) : x(_x), y(_y) {}

  bool operator==(const Punto& p) const { return x == p.x && y == p.y; }

  bool operator<(const Punto& p) const {
    if (x != p.x)
      return x < p.x;
    return y < p.y;
  }

  bool e_valido() const { return x >= 0 && y >= 0; }
};

struct Percorso {
  std::vector<Punto> punti;
  double costo;

  Percorso() : costo(0) {}
};

struct Bordo_e_uscita {
  Punto uscita;
  Percorso bordo;
  senso_di_percorrenza direzione;
};

class Griglia {
private:
  std::vector<std::vector<TipoCella>> griglia;
  int larghezza;
  int altezza;
  std::vector<std::vector<int>> id_ostacolo;

public:
  Griglia(const std::vector<std::vector<TipoCella>>& griglia);

  bool e_oltrepassabile(int x, int y) const;
  bool e_oltrepassabile(const Punto& p) const;
  int ottieni_larghezza() const;
  int ottieni_altezza() const;
  void controlla_id_ostacolo();
  int ottieni_id_ostacolo(int x, int y) const;
  int ottieni_id_ostacolo(const Punto& p) const;
  void rendi_non_oltrepassabile(int y, int x);
Griglia (int larghezza, int altezza): griglia(altezza, std::vector<TipoCella>(larghezza, TipoCella::oltrepassabile)){}
private:
  void riempi(int start_x, int start_y, int id);
};

// funzioni
std::vector<Punto> linea_dritta_con_bresenham(const Punto& da, const Punto& a);
Punto trova_primo_ostacolo_sulla_linea(const Punto& A, const Punto& B,
                                 const Griglia& griglia);
Punto trova_ultimo_punto_prima_del_primo_ostacolo(const Punto& A, const Punto& B,
                                            const Griglia& griglia);
bool e_libero_fino_a_punto_di_arrivo(const Punto& P, const Punto& B, const Griglia& griglia);
bool e_libero_spazio_in_direzione_arrivo(const Punto& P, const Punto& B,
                                     const Griglia& griglia, int id_ostacolo);

Bordo_e_uscita segui_il_bordo_monodirezione(const Punto& partenza,
                                         const Punto& destinazione,
                                         const Griglia& griglia, int id_ostacolo,
                                         senso_di_percorrenza direzione);

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d);

double calcola_lunghezza_percorso(const std::vector<Punto>& percorso);

std::vector<std::vector<Punto>>
semplifica_percorso_all_indietro(const std::vector<Punto>& percorso, const Griglia& griglia);

std::vector<std::vector<Punto>>
semplifica_percorso_all_indietro_fino_a_stabilizzazione(const std::vector<Punto>& percorso, const Griglia& griglia);

void costruisci_percorsi_ricorsivo(const Punto& corrente, const Punto& destinazione,
                         const Griglia& griglia,
                         std::vector<Punto>& percorso_parziale,
                         double costo_parziale,
                         std::vector<Percorso>& percorsi_output, int profondita);

std::vector<Percorso> trova_percorsi(const Punto& A, const Punto& B,
                              const Griglia& griglia);

std::vector<Percorso> filtra_percorsi_per_lunghezza(std::vector<Percorso>& percorsi);

std::vector<Percorso> 
trova_percorsi_con_algoritmo_completo(const Punto& A, const Punto& B,
                 const std::vector<std::vector<TipoCella>>& griglia);
} // namespace pf
#endif