class Convolutional_Neural_Networks{
private:
	char **type_layer;

	int batch_size;
	int number_layers;
	int number_memory_types;

	int *kernel_width;
	int *kernel_height;
	int *map_width;
	int *map_height;
	int *number_maps;
	int *stride_width;
	int *stride_height;

	double *****weight;

	double ******neuron;
	double ******derivative;

	// Variables for Batch Normalization
	double epsilon;

	double **gamma;
	double **beta;
	double **mean;
	double **variance;
	double **sum_mean;
	double **sum_variance;
	// *********************************

	void Activate(char option[], int layer_index, int map_index);
	void Adjust_Parameter(int layer_index, int map_index);
	void Backpropagate(int layer_index, int map_index);
	void Differentiate(int layer_index, int map_index, double learning_rate, double **target_output);
	void Feedforward(int layer_index, int map_index);
	void Softmax(int layer_index);

	void Batch_Normalization_Activate(char option[], int layer_index, int map_index);
	void Batch_Normalization_Adjust_Parameter(int layer_index, int map_index);
	void Batch_Normalization_Differentiate(int layer_index, int map_index);

	void Resize_Memory(int batch_size);

	bool Access_Memory(int type_index, int layer_index);
public:
	Convolutional_Neural_Networks(char **type_layer, int number_layers, int map_width[], int map_height[], int number_maps[]);
	~Convolutional_Neural_Networks();

	void Initialize_Parameter(int seed, double scale, double shift);
	void Load_Parameter(char path[]);
	void Save_Parameter(char path[]);
	void Test(double input[], double output[]);

	double Train(int batch_size, int number_training, double epsilon, double learning_rate, double **input, double **target_output);
};
