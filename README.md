![alt text](https://sites.lsa.umich.edu/ksmoore/wp-content/uploads/sites/630/2018/06/TacomaNarrows.jpg)

# Intro_to_ComplexSystems-Kuramoto
Spontaneous synchronisation for the Kuramoto model - Assignment for the Introduction to Complex Systems Course @ Utrecht University, Dec'19-Jan'20. Main simulations will run with a C code, plotting and analysis on Python

### To do:
  - Implement Python pipeline for plots

### Ideas:
  - Average of Fourier analysis for different K-s in order to relate coupling strenght to characteristic oscillations of the Kuramoto systems (note: averaging and then Fourier would just result in suppression of modes..)
  - Proof of theoretical questions with low N calculations validated by simulation
  
### Issues:
  - Check why K value is scaled!
  - Why std of imaginary part of order parameter is always 0..?!

### To say in the report:
  - Chosing coding language --> benchmarks
  - Organization of workflow, C-->Python in standard part, Python-->C-->Python in bonus part
  - First Euler approach: full hamiltonian, slower, nonetheless used in bonus part
  - Second Euler approach: mean field --> lower computational times --> averaging EVERY results with good enough parameters 
