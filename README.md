[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-dark-v6.png#gh-dark-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx#gh-dark-mode-only)
[<img src="https://assets.signaloid.io/add-to-signaloid-cloud-logo-light-v6.png#gh-light-mode-only" alt="[Add to signaloid.io]" height="30">](https://signaloid.io/repositories?connect=https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx#gh-light-mode-only)


# Example: Accelerated Quantum Phase Estimation (AQPE) without Signaloid's Infrastructure

## Summary
This repository contains an implementation of the classical part of the Accelerated Quantum Phase Estimation (AQPE) subroutine of the Accelerated Variational Quantum Eigensolver (AVQE) based on work by Cruise et al. [^0] and Wiebe et al. [^1]. For phase estimation, the implementation uses an efficient approximation to Bayesian inference called Reduction Filtering Phase Estimation (RFPE).

The repository [Signaloid-Demo-Quantum-AQPE](https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE) contains an implementation of AQPE that takes advantage of the capabilities of Signaloid's compute engine and replaces RFPE with a Bayesian inference operation provided by Signaloid's compute engine.

## The AQPE Algorithm
The AQPE algorithm is a method to estimate the eigenphase $\phi$ corresponding to the eigenvector $\ket{\phi}$ of a single-qubit quantum gate represented by the unitary matrix $U$, that is, $U\ket{\phi} = e^{i\phi}\ket{\phi}$. AQPE uses the quantum circuit shown below to obtain the evidence that it then uses in estimating the value of $\phi$.

<img width="800" alt="image" src="https://user-images.githubusercontent.com/115564080/235872639-cb6866b2-cfcb-421f-b2cf-538110ca43fe.png">

AVQE is a generalization of the standard Variational Quantum Eigensolver (VQE). AVQE replaces the Quantum Expectation Estimation (QEE) subroutine with the AQPE subroutine and it offers a bridge between AVQE and the classical QPE algorithm by Kitaev et al. [^2]. It does that by introducing a parameter, $\alpha \in [0,1]$, which enables a tradeoff between the required quantum circuit depth $D$ and the number of required quantum circuit measurements $N$, to acheive a specified precision $p$ in estimating the value of the phase $\phi$.

| Algorithm | Quantum Circuit Depth, $D$ | Number of Quantum Circuit Measurements, $N$ |
| ------ | --- | --- |
| AVQE | $O(p^{-\alpha})$ | $O(p^{{-2(1 - \alpha)}})$ for $\alpha \in [0,1)$, $O(\mathrm{log}(p^{-1}))$ for $\alpha = 1$|
| VQE | $O(1)$ | $O(p^{-2})$ |
| Kitaev's QPE| $O(p^{-1})$ | $O(1)$ |

The table above shows the requirements for the quantum circuit depth $D$ and the number of quantum circuit measurements $N$ of each algorithm (AVQE, VQE, and Kitaev's QPE). For the quantum circuit depth, AVQE requires that $D = O(p^{-\alpha})$ for $\alpha \in [0,1]$, and for the number of quantum circuit measurements, AVQE requires that $N = O(p^{{-2(1 - \alpha)}})$ for $\alpha \in [0,1)$ and $N = O(\mathrm{log}(p^{-1}))$ for $\alpha = 1$. When $\alpha = 0$, AVQE requirements exactly match that of VQE which has $D = O(1)$ and $N = O(p^{-2})$. On the other hand, when $\alpha = 1$, AVQE requirements match (upto a logarithmic factor for $N$) that of Kitaev's QPE which has $D = O(p^{-1})$ and $N = O(1)$.

With the contemporary NISQ-era quantum computers being capable of maintaining qubit coherence for quantum circuit depths of only few hundreds, implementing Kitaev's QPE approach is out of question for low values of $p$ (e.g., $p \leq 10^{-3}$ for which $D \geq O(10^3)$). On the other hand, the high number of required quantum circuit measurements for VQE makes it prohibitive for low values of $p$ (e.g., $p \leq 10^{-3}$ for which $N \geq O(10^6)$). AVQE allows one to achieve a trade-off between $D$ and $N$, where one would choose $\alpha > 0$ for a given maximum depth $D_{\mathrm{max}}$ that an available NISQ computing machine can sustain and would require less number of quantum circuit measurements compared to VQE.

## An interesting use-case for NISQ era
After clicking on the "add to signaloid.io" button above, click on the <img width="56" alt="Screenshot 2023-06-29 at 22 55 31" src="https://github.com/signaloid/Signaloid-Demo-Quantum-AQPE-NoUx/assets/86417/eb19828c-789e-46e2-ba93-269a8ae64131"> button to set the command-line arguments to `-p 1e-4 -a 0.5`. This sets $\alpha = 0.5$ and the estimation precision to $10^{-4}$. For these values, the required quantum circuit depth is $D = 100$ and the required number of shots or measurements of the quantum circuit on a quantum computer is $N = 39996$. This depth is achievable by NISQ-era quantum computers.

## Usage
```
[-i [path_to_input_csv_file : str] (Default: '../inputs/input.csv')] (Default: stdin)
[-o [path_to_output_csv_file : str] (Default: './sd0/aqpeOutput.csv')] (Default: stdout)
[-t <target_phase : double in [-pi, pi]>] (Default: pi / 2)
[-p <precision_in_phase_estimation : double in (0, inf)>] (Default: 0.01)
[-a <alpha : double in [0,1]>]  (Default: 1.0)
[-n <number_of_evidence_samples_per_iteration : int in [1, inf)>] (Default: 1 / precision^{alpha})
[-h] (Display this help message.)
```

## Repository Tree Structure
```
.
├── LICENSE
├── README.md
├── gsl
├── include
│   └── gsl
├── libs
│   ├── libgsl.a
│   └── libgslcblas.a
└── src
    ├── README.md
    ├── main.c
    ├── utilities.c
    └── utilities.h
```

## References
[^0]: J. R. Cruise, N. I. Gillespie, and B. Reid: Practical Quantum Computing: The value of local computation. arXiv:2009.08513 [quant-ph], Sep. 2020. Available Online: http://arxiv.org/abs/2009.08513.

[^1]: N. Wiebe and C. Granade: Efficient Bayesian Phase Estimation. Phys. Rev. Lett. 117, 010503, June 2016. Avlaialable Online: https://arxiv.org/abs/1508.00869.

[^2]: A. Y. Kitaev, A. Shen, and M. N. Vyalyi: Classical and Quantum Computation. American Mathematical Society, 2002.
