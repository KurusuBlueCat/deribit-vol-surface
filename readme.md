# Final Project for QF633, Cpp for Financial Engineering.

Group Member:

- Miti Nopnirapath
- Lee Chew Peng Caden
- Lee Hiao Huang Lora

# Setting Up

The project requires cmake to compile. 
Executables produced are (relative to build folder):

1. project/step1
2. project/step2
3. project/step3
4. project/unittestsGradientMaker (this test numerical differencing against analytical values for Rosenbrock function)
5. project/Solver/LBFGSpp/examples/test_lbfgs (example-rosenbrock-box.cpp)
6. project/Solver/LBFGSpp/examples/test_lbfgs_2 (example-rosenbrock-bracketing.cpp)
7. project/Solver/LBFGSpp/examples/test_lbfgs_3 (example-rosenbrock-comparison.cpp)

# Required Data

Don't forget to provide data in the folder TestData. The schema are as follows:

Filetype: CSV

Columns: 
1. contractName
2. time
3. msgType
4. priceCcy
5. bestBid
6. bestBidAmount
7. bestBidIV
8. bestAsk
9. bestAskAmount
10. bestAskIV
11. markPrice
12. markIV
13. underlyingIndex
14. underlyingPrice
15. interest_rate
16. lastPrice
17. open_interest
18. vega
19. theta
20. rho
21. gamma
22. delta

Example Rows:

BTC-24JUN22-29000-P, 2022-05-15T00:00:00.227Z, update, BTC, 0.08, 2.9, 75.12, 0.0815, 9.3, 76.29, 0.0806, 75.57, BTC-24JUN22, 30122.38, 0, 0.0885, 163.8, 38.44607, -36.01751, -15.69622, 0.00005, -0.39098
