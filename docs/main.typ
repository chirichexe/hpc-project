#import "@preview/diatypst:0.8.0": *

#let weak_mpi = csv("csv/weak_mpi.csv")
#let strong_mpi = csv("csv/strong_mpi.csv")
#let weak_omp = csv("csv/strong_openmp.csv")
#let strong_omp = csv("csv/weak_openmp.csv")
#let weak_multinode_mpi = csv("csv/weak_multinode_mpi.csv")
#let strong_multinode_mpi = csv("csv/strong_multinode_mpi.csv")

#let scaling_table(data) = {
  let fmt(val) = {
    if type(val) == str {
      let n = float(val)
      str(calc.round(n, digits: 4))
    } else {
      str(calc.round(val, digits: 4))
    }
  }

  align(center + horizon)[
    #block(width: 95%)[
      #set text(size: 1.1em)
      #table(
        columns: (1fr, 1fr, 1.5fr, 1.5fr, 1.2fr, 1.2fr),
        inset: (x: 12pt, y: 5pt), 
        align: center + horizon,
        stroke: 0.5pt + gray.lighten(40%),
        fill: (x, y) => 
          if y == 0 { blue.darken(70%) }
          else if calc.even(y) { gray.lighten(94%) } 
          else { white },

        [*#text(fill: white)[P]*], 
        [*#text(fill: white)[N]*], 
        [*#text(fill: white)[T. Medio (s)]*], 
        [*#text(fill: white)[Std. dev]*], 
        [*#text(fill: white)[Speedup]*], 
        [*#text(fill: white)[Efficiency]*],
        
        ..data.slice(1).map(row => (
          row.at(0), 
          row.at(1), 
          fmt(row.at(2)), 
          fmt(row.at(3)), 
          fmt(row.at(4)), 
          fmt(row.at(5))
        )).flatten()
      )
    ]
  ]
}

#show: slides.with(
  title: "Binarizzazione di una matrice", 
  subtitle: "Progetto - Sistemi Concorrenti e Paralleli M",
  date: "A.A. 2025-2026",
  authors: ("Davide Chirichella - 0001222371"),
  ratio: 16/9,
  layout: "medium",
  title-color: blue.darken(70%),
  toc: true,
)

= Proposta di progetto

== Obiettivi
Trasformare una matrice *quadrata* in una matrice *binaria* basata sulla media locale di ogni elemento.

- *Matrice di Input ($A$)* di dimensione $N times N$, con $N >= 2000$.
- *Matrice di Output ($T$)* di dimensione $N times N$ con valori  $ {0, 1}$.

#v(1em)

#grid(
  columns: (2fr, 1.4fr), 
  gutter: 2em,
  
  // Prima colonna: Spiegazione
  align(left + top)[
    1. *Media dell'intorno ($m_(i,j)$):* Media aritmetica degli elementi appartenenti all'intorno $I_(i,j)$ con la formula:
    $ m_(i,j) = 1/9 sum_(x=i-1)^(i+1) sum_(y=j-1)^(j+1) a_(x,y) $ 

    2. *Sogliatura binaria:*
    - Se $a_(i,j) > m_(i,j)$ allora $t_(i,j) = 1$
    - Se $a_(i,j) <= m_(i,j)$ allora $t_(i,j) = 0$
  ],

  // Seconda colonna: Immagine
  align(center + horizon)[
    #image("drawings/hpc-proj-intorno.drawio.png", width: 70%)
  ]
)


= Implementazione Seriale 

== Algoritmo

Si allocano le matrici secondo la *Memorizzazione Row-Major*. L’elemento in posizione $(i, j)$ può essere acceduto linearmente come: `A[i * N + j]`, e viene allocata nel seguente modo:

```c
int *A = malloc(N * N * sizeof(int));
int *T = malloc(N * N * sizeof(int));
```

- Si cicla lungo le righe e le colonne, definendo la somma dei valori `sum` e il numero di elementi  `count`:
```c
for (i = 0; i < N; i++) {
  for (j = 0; j < N; j++) {
    sum = 0;
    count = 0;
```

#pagebreak()

E si definiscono i confini dell'intorno, stabilendo "fino a quanto" si può uscire dalla cella corrente:

```c
zmin = (i > 0) ? i - 1 : i;
zmax = (i < N - 1) ? i + 1 : i;
wmin = (j > 0) ? j - 1 : j;
wmax = (j < N - 1) ? j + 1 : j;
```
\

Si cicla successivamente tra i valori dell'intorno:

```c
for ( z = zmin; z <= zmax; z++) {
    for ( w = wmin; w <= wmax; w++) {
        sum += A[z * N + w];
        count++;

```
#pagebreak()

Infine, si calcola la soglia binaria: 

```c
T[i * N + j] = (A[i * N + j] * count > sum) ? 1 : 0
```

*Nota*: È presente una leggera *ottimizzazione*, ovvero l'uso della formula ($A[i][j] * "count" > "sum"$) al posto della divisione ($"sum""/""count"$).


= Implementazione Parallela: MPI

== Distribuzione del carico di lavoro

*Considerazioni*:

- MPI adotta un modello a *scambio di messaggi*: necessità di comunicazione tra processi per elementi dell'intorno.

* Calcolo dimensioni delle sotto-matrici *


Poiché non è garantito che la dimensione della matrice $N$ sia un multiplo esatto del numero di processi:

#grid(
  columns: (1fr, 1fr),
  gutter: 15pt,
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`base_rows`*: Ogni processo riceve almeno `N / num_proc` righe.
  ],
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`extra_rows`:* Le righe rimanenti (`N % num_proc`) vengono ridistribuite.
  ]
)
#pagebreak()

#grid(
  [
    * Calcolo elementi di ogni processo, * inviati in Broadcast:
    #grid(
      columns: (1fr, 1fr),
      gutter: 15pt,
      block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
        *`sendcounts`*: Memorizza il numero totale di elementi assegnati a ogni processo.
      ],
      block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
        *`senddispls`:* L’offset iniziale di ciascun processo.
      ]
    )

  ]
)
#align(center)[
  
      #image("drawings/hpc-proj-bcast.drawio.png", width: 40%)
]

== Allocazione e scambio delle Ghost Rows

Ogni processo alloca quindi la propria porzione di righe: 

```c
my_rows = sendcounts[my_rank] / N
```
Dovrà tuttavia allocare due righe extra:

#grid(
  columns: (1fr, 2fr),
  column-gutter: 2em,
  align: horizon,
  [
    - #strong[Upper Ghost Row]: Ospita l'ultima riga del processo precedente ($P_(k-1)$), se esiste.
    \
    - #strong[Lower Ghost Row]: Ospita la prima riga del processo successivo ($P_(k+1)$), se esiste.
  ],
  [
    #set align(center)
    #image("drawings/hpc-proj-matrix-scatter.drawio.png", width: 100%)
  ]
)

Successivamente, ogni processo:
1. Determina chi sono processi adiacenti:
```c
int up   = (my_rank > 0)            ? my_rank - 1 : MPI_PROC_NULL;
int down = (my_rank < num_proc - 1) ? my_rank + 1 : MPI_PROC_NULL;
```
2. Alloca la struttura dati per propria porzione di matrice con due righe extra:
```c
int total_rows = my_rows + 2;
int *my_A_plus_ghosts = calloc(total_rows * N, sizeof(int));
```

3. Definisce un puntatore per ogni sezione della memoria allocata:
```c
int *upper_ghost = my_A_plus_ghosts;                     // row -1
int *local_data  = my_A_plus_ghosts + N;                 // rows [0 ... my_rows-1]
int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * N; // row + my_rows
```

    4. Il Master distribuisce le porzioni della matrice ai vari processi tramite una `MPI_Scatterv`:

#grid(
  columns: (1.5fr, 2fr), 
  gutter: 1em,
  
  align(center + horizon)[
     #image("drawings/hpc-proj-scatterv.drawio.png")
  ],
  
  align(horizon)[

    ```c
    MPI_Scatterv(
        A, sendcounts, senddispls, MPI_INT,
        local_data, sendcounts[my_rank], MPI_INT,
        0, MPI_COMM_WORLD
    );
  ```
  ]
)


*Approcci possibili per la comunicazione delle righe:*

  - `MPI_Recv` (bloccante) + `MPI_Ssend`: implementano una comunicazione sincrona di tipo *rendez-vous*.
  - `MPI_Irecv` + `MPI_Isend`: permettono comunicazioni *asincrone*, richiedono `MPI_Wait`.
  - `MPI_Sendrecv`: esegue simultaneamente invio e ricezione, ottimizzando la sincronizzazione.


Di seguito è mostrata l'implementazione mediante `Ssend` e `Recv`:

#grid(
  columns: (2.2fr, .7fr),
  column-gutter: 10pt,
  align: horizon,
  [
1. Ogni processo (tranne il primo) invia la propria riga di bordo superiore al vicino `up`:
    ```c
  if (my_rank > 0 && my_rows > 0) {
      MPI_Ssend(local_data, N, MPI_INT, up, UP_TAG, MPI_COMM_WORLD);
      MPI_Recv(upper_ghost, N, MPI_INT, up, DOWN_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  }
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-2.drawio.png", width: 100%)]
)

2. Successivamente, si calcola l'indice di memoria dove inizia l'ultima riga locale per l'invio al processo sottostante:

```c
int send_down_offset = (my_rows > 0 ? (my_rows - 1) * N : 0);
```
#pagebreak()

3. L'invio tramite `Ssend` sincronizza i processi: una volta completata la trasmissione della riga di confine, il processo viene "sbloccato" per la ricezione dal nodo superiore della Ghost Row necessaria:

#grid(
  columns: (2.7fr, 1.1fr),
  column-gutter: 10pt,
  align: horizon,
  [
    ```c
if (my_rank < num_proc - 1 && my_rows > 0) {
    if (sendcounts[down] > 0) {
        MPI_Recv(lower_ghost, N, MPI_INT, down, UP_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Ssend(local_data + send_down_offset, N, MPI_INT, down, DOWN_TAG, MPI_COMM_WORLD);
    }
}
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-1.drawio.png", width: 70%)]
)

== Calcolo elementi dell'intorno e sogliatura

Dopo lo scambio dei dati, ogni processo itera sulla propria porzione di matrice. Si ricorda che:

#table(
  columns: (2.3fr, 2.3fr),
  inset: 3pt,
  stroke: gray.lighten(60%),

  [*Indice (riga logica)*], [*Contenuto*],
  [`riga 0`], [Ghost row superiore],
  [`righe 1 … my_rows`], [Righe di dati locali],
  [`riga my_rows + 1`], [Ghost row inferiore],
)

  Per compensare la presenza delle Ghost Rows, l'indice locale deve quindi essere traslato verticalmente:
```c
int zmin = (i == 0 && my_rank == 0) ? 0 : -1;                     // limite superiore
int zmax = (i == my_rows - 1 && my_rank == num_proc - 1) ? 0 : 1; // limite inferiore
```
  E orizzontalmente:
```c
int wmin = (j > 0) ? -1 : 0;    // limite destro
int wmax = (j < N - 1) ? 1 : 0; // limite sinistro
```


  #grid(
  columns: (2.2fr, 1fr), 
  gutter: 1em,
  
  align(left + horizon)[
      ```c
for (int dz = zmin; dz <= zmax; dz++) {
  int rowIndex = i + 1 + dz;
  
  for (int dw = wmin; dw <= wmax; dw++) {
    int colIndex = j + dw;
    
    sum += my_A_plus_ghosts[rowIndex * N +
    colIndex];
    
    count++;
  }
}

my_T[i * N + j] = (local_data[i * N + j] * count > sum)? 1 : 0;
```
  ],
  align(horizon)[
     #image("drawings/hpc-proj-matrix-for-cycle.drawio.png")
  ]
)

== Raccolta risultati

Si libera la memoria allocata per la matrice locale con le Ghost Rows:

```c
free(my_A_plus_ghosts);
```
Il nodo Master invoca la primitiva `MPI_Gatherv` per collezionare i risultati parziali:
```c
MPI_Gatherv(
    my_T, (my_rows * N), MPI_INT,           
    T_raw, sendcounts, senddispls, MPI_INT,
    0, MPI_COMM_WORLD
);
```

= Implementazione Parallela: OpenMP

== Considerazioni generali

L'implementazione OpenMP è significativamente più semplice a causa della sua gestione dei dati:

- Approccio *Shared Memory*, ogni thread può leggere direttamente l'intorno di un punto.

- Il compilatore si occupa di parallelizzare il codice mediante clausole singole `#pragma`

Poichè OpenMP segue il modello _Cobegin-Coend_, il Master thread dovrà creare dei worker thread al momento della direttiva `parallel`.

#align(center)[#image("drawings/hpc-proj-cobegin-coend.drawio.png", width: 70% )]

== Scelte implementative

Le scelte adottate per l'implementazione dell'algoritmo parallelo hanno riguardato: 

- *Visibilità delle Variabili*: 
  - Matrici $A$ e $T$: allocate fuori dal ciclo e dichiarate `shared` tra tutti i thread.
  - Variabili di supporto al calcolo locale ($i, j, "sum", "count", "etc..."$): dichiarate `private` (andranno successivamente inizializzate dai singoli thread).

  #align(center)[#image("drawings/hpc-proj-visibilita-variabili.drawio.png", width: 70% )]

- *Clausola `for`*: Permette di distribuire le iterazioni di un ciclo tra i thread, e si è scelto di farlo solo per il ciclo "esterno" (che itera sulle righe).

- *Scheduling dei thread*: è stato scelto lo `scheduling(static)` perché il carico computazionale di ogni iterazione è predeterminato e ogni thread lavora su righe contigue.

#grid(
  columns: (30%, 70%),
  gutter: 10pt,
  [
     #set align(horizon)
     - *Nota*: Se il numero di righe non è divisibile per i thread, OpenMP assegna le iterazioni rimanenti ai thread con rank *inferiore* per coprire l'intero dominio.
     
  / Esempio: Matrice: $4 times 4$\ Numero di thread: $3$
  ],
  [
    #set align(right)
    #image("drawings/hpc-proj-for.drawio.png", width: 100%)
  ]
)

== Algoritmo

Si definiscono le clausole OpenMP che permettono di stabilire:
- L'assegnazione statica delle righe nel ciclo *for*;
- Numero di *thread* per processo (come parametro del programma);
- Variabili *condivise*;
- Variabile *private* (non inizializzate).

```c
#pragma omp parallel for num_threads(num_threads) schedule(static) \
shared(A, T, N) private(i, j, sum, count, zmin, zmax, wmin, wmax, z, w)
```

L'implementazione dell'algoritmo è la medesima della versione sequenziale:

```c
for (i = 0; i < N; i++) {
  for (j = 0; j < N; j++) {
    sum = 0;
    count = 0;
    // ...
```

= Benchmark
== Setup sperimentale

*Obiettivo*

Valutare la scalabilità dell'algoritmo al variare delle risorse di calcolo e della dimensione del problema.

*Analisi Effettuate* 


1. *Strong Scaling*: Dimensione del problema fissa ($N = 10.000$), unità di calcolo parallelo crescente ($P = 1 ... 48$).

2. *Weak Scaling*: Dimensione del problema ($N = 5000 sqrt(P)$) e unità di calcolo parallelo ($P = 1 ... 48$) crescenti.

*Configurazioni testate:*

1. *MPI intra-nodo* (più processi MPI indipendenti su un singolo nodo)

2. *MPI inter-nodo* (un processo MPI per nodo, con aumento progressivo del numero di nodi utilizzati)

3. *OpenMP* inter-nodo.

#pagebreak()

Le direttive *Slurm* utilizzate sulla macchina _Galileo-100_ del _Cineca_ sono le seguenti: 

#table(
  columns: (2fr, 0.8fr, 1fr, 1.6fr, 1.1fr),
  align: left,
  inset: (x: 10pt, y: 6pt),
  stroke: 0.5pt + gray.lighten(40%),
  fill: (x, y) =>
    if y == 0 { blue.darken(70%) }
    else if calc.even(y) { gray.lighten(94%) }
    else { white },

  [*#text(fill: white)[Configurazione]*],
  [*#text(fill: white)[Nodes]*],
  [*#text(fill: white)[Tasks]*],
  [*#text(fill: white)[CPU per Task]*],
  [*#text(fill: white)[N]*],

  [*MPI*, Strong scaling inter-nodo],
  [1],
  [$1 ... 48$],
  [1],
  [$10.000$],

  [*MPI*, Weak scaling  inter-nodo],
  [1],
  [$1...48$ ],
  [1],
  [$5000 sqrt(P)$],

  [*MPI*, Strong scaling  intra-nodo],
  [$1...24$],
  [$1$ per nodo],
  [1],
  [$10.000$],


  [*MPI*, Weak scaling intra-nodo],
  [$1...24$],
  [$1$ per nodo],
  [1],
  [$5000 sqrt(P)$],

  [*OpenMP*, Strong scaling],
  [1],
  [1],
  [48 ($1...48$ thread)],
  [$10.000$],

  [*OpenMP*, Weak scaling],
  [1],
  [1 ],
  [48 ($1...48$ thread)],
  [$5000 sqrt(P)$],
)


== MPI
=== Strong Scaling inter-nodo

- All'aumentare di $P$, il numero di righe elaborate da ogni processo diminuisce, aumentando l'*overhead*.



#scaling_table(strong_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_strong.png", width: 80%)]
  ),
  caption: [Strong Scaling inter-nodo - MPI],
) <fig-omp-weak>

 

=== Weak Scaling inter-nodo

- Nel caso di *weak scaling* si osserva un comportamento analogo.

#scaling_table(weak_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_weak.png", width: 80%)]
  ),
  caption: [Weak Scaling inter-nodo - MPI],
) <fig-omp-weak>

=== Strong Scaling intra-nodo

- La similitudine tra il comportamento MPI *intra-nodo* e *inter-nodo* conferma il predominio dell’overhead di comunicazione rispetto al lavoro computazionale, che limita lo scaling.


#scaling_table(strong_multinode_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_multinode_strong.png", width: 75%)]
  ),
  caption: [Strong Scaling intra-nodo - MPI],
) <fig-omp-weak>

=== Weak Scaling intra-nodo

- Si osserva anche qui un comportamento analogo al caso precedente, con benefici di scaling limitati.

#scaling_table(weak_multinode_mpi)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/mpi_multinode_weak.png", width: 75%)]
  ),
  caption: [Weak Scaling intra-nodo - MPI],
) <fig-omp-weak>

== OpenMP

=== Strong Scaling

- OpenMP mantiene un'efficienza estremamente elevata, sempre superiore al *90%*.

#scaling_table(weak_omp)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/omp_strong.png", width: 80%)]
  ),
  caption: [Strong Scaling - OpenMP],
) <fig-omp-weak>



=== Weak Scaling

- Il tempo di esecuzione rimane pressoché costante ($approx 0.87s$), ogni core riceve   la stessa *quantità* di lavoro. 

#scaling_table(strong_omp)

#figure(
  grid(
    columns: (1fr),
    gutter: 10pt,
    align(center)[#image("plot/omp_weak.png", width: 80%)]
  ),
  caption: [Weak Scaling - OpenMP],
) <fig-omp-weak>

#set align(center + horizon)

= Conclusioni

== Risultati ottenuti

#grid(
  columns: (1fr, 1fr),
  gutter: 20pt,
  align(left)[
    === OpenMP (Shared Memory)
    - *Speedup*: Quasi ideale (Efficienza sempre $> 95%$).
    
    - *Punti di forza*: Accesso diretto ai dati, zero overhead di comunicazione.
    
    - *Criticità*: Vincolato alle risorse di un singolo nodo.
  ],
  align(left)[
    === MPI (Distributed Memory)
    - *Speedup*: Limitato (Efficienza anche $< 20%$).
    
    - *Punti di forza*: Permette di eseguire su cluster multi-nodo per $N$ estremamente grandi.
    
    - *Criticità*: Performance limitate dall'overhead di comunicazione.
  ]
)

#v(2em)

#block(
  fill: luma(230),
  inset: 15pt,
  radius: 5pt,
  [
    In base ai calcoli effettuati, *OpenMP* risulta la soluzione più efficiente per la natura del problema. *MPI* è necessario solo quando la memoria del singolo nodo non è più sufficiente. Soluzione possibile: *Hybridization* 
  ]
)