# Perceptron

- To classify MNIST handwritten digits, following files are required from http://yann.lecun.com/exdb/mnist/
  - train-images.idx3-ubyte
  - train-labels.idx1-ubyte
  - t10k-images.idx3-ubyte
  - t10k-labels.idx1-ubyte

- The number of neurons in the input and output layer is determined by number_neurons in the main.cpp.
  ```C++
  86: int number_neurons[] = {28 * 28, 10};
  ```

## MNIST classification result
![result](/Perceptron/result.PNG)
