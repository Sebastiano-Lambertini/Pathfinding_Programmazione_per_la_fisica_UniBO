#ifndef PATH_H
#define PATH_H

#include <cmath>
#include <vector>

namespace pf {
enum senso_di_percorrenza { orario, antiorario };
enum class TipoCella { oltrepassabile, non_oltrepassabile };
struct Punto {
  int x;
  int y;

  Punto() : x(0), y(0) {}
  Punto(int _x, int _y) : x(_x), y(_y) {}

  bool operator==(const Punto &p) const { return x == p.x && y == p.y; }

  bool operator<(const Punto &p) const {
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
  std::vector<TipoCella> griglia;
  int larghezza;
  int altezza;
  std::vector<int> id_ostacolo;
  double fattore_scala;
  int min_x;
  int max_x;
  int min_y;
  int max_y;
  Punto arrivo;
  Punto partenza;

public:
  Griglia(const std::vector<TipoCella> &griglia);

  bool e_oltrepassabile(int x, int y) const;
  bool e_oltrepassabile(const Punto &p) const;
  int ottieni_larghezza() const;
  int ottieni_altezza() const;
  void controlla_id_ostacolo();
  int ottieni_id_ostacolo(int x, int y) const;
  int ottieni_id_ostacolo(const Punto &p) const;
  void rendi_non_oltrepassabile(int x, int y);
  Punto ottieni_arrivo() const;         ///////////////////////da fare
  Punto ottieni_partenza() const;       ///////////////777
  double ottieni_fattore_scala() const; ////////////////////da fare
  void inserisci_scala(double fattore_scala_calcolato);//////////////////////////////7
    int ottieni_max_x() const;//////////////////////////
  int ottieni_max_y() const;/////////////////////
  int ottieni_min_x() const;////////////////////////
  int ottieni_min_y() const;//////////////////////////
  void inserisci_max_y(int max_y_calcolato);
  void inserisci_min_y(int min_y_calcolato);
  void inserisci_max_x(int max_x_calcolato);
  void inserisci_min_x(int min_x_calcolato);
  void inserisci_partenza(int x, int y);
  void inserisci_arrivo(int x, int y);
  void inserisci_partenza(const Punto &p);
  void inserisci_arrivo(const Punto &p);
void inserisci_larghezza(int larghezza_calcolata);///////////77
void inserisci_altezza(int altezza_calcolata);////////////////77
void forma_griglia_vettore(int altezza, int larghezza);
void dilata_ostacoli();
  Griglia(int larghezza, int altezza)
      : griglia(altezza*larghezza, TipoCella::oltrepassabile) {}
Griglia() : griglia(), larghezza(0), altezza(0), fattore_scala(1.0),
            min_x(0), max_x(0), min_y(0), max_y(0), arrivo(), partenza() {}
private:
  void riempi(int start_x, int start_y, int id);
  int indice_monodimensionale(int x, int y) const;
};

// funzioni
std::vector<Punto> linea_dritta_con_bresenham(const Punto &da, const Punto &a);
Punto trova_primo_ostacolo_sulla_linea(const Punto &A, const Punto &B,
                                       const Griglia &griglia);
Punto trova_ultimo_punto_prima_del_primo_ostacolo(const Punto &A,
                                                  const Punto &B,
                                                  const Griglia &griglia);
bool e_libero_fino_a_punto_di_arrivo(const Punto &P, const Punto &B,
                                     const Griglia &griglia);
bool e_libero_spazio_in_direzione_arrivo(const Punto &P, const Punto &B,
                                         const Griglia &griglia,
                                         int id_ostacolo);

Bordo_e_uscita segui_il_bordo_monodirezione(const Punto &partenza,
                                            const Punto &destinazione,
                                            const Griglia &griglia,
                                            int id_ostacolo,
                                            senso_di_percorrenza direzione);

senso_di_percorrenza direzione_opposta(senso_di_percorrenza d);

double calcola_lunghezza_percorso(const std::vector<Punto> &percorso);

std::vector<std::vector<Punto>>
semplifica_percorso_all_indietro(const std::vector<Punto> &percorso,
                                 const Griglia &griglia);

std::vector<std::vector<Punto>>
semplifica_percorso_all_indietro_fino_a_stabilizzazione(
    const std::vector<Punto> &percorso, const Griglia &griglia);

void costruisci_percorsi_ricorsivo(
    const Punto &corrente, const Punto &destinazione, const Griglia &griglia,
    std::vector<Punto> &percorso_parziale, double costo_parziale,
    std::vector<Percorso> &percorsi_output, int profondita);

std::vector<Percorso> trova_percorsi(const Punto &A, const Punto &B,
                                     const Griglia &griglia);

std::vector<Percorso>
filtra_percorsi_per_lunghezza(std::vector<Percorso> &percorsi);

std::vector<Percorso>
trova_percorsi_con_algoritmo_completo(const Punto &A, const Punto &B,
                                       Griglia &griglia);
} // namespace pf
#endif