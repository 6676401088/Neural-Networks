#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CNN.h"

void Convolutional_Neural_Networks::Activate(char option[], int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	if(type_layer[i][0] == 'C'){
		if(strstr(type_layer[i], "bn")){
			Batch_Normalization_Activate(option, layer_index, map_index);
		}

		for(int h = 0;h < batch_size;h++){
			double mask = 1;

			if(strstr(type_layer[i], "do")){
				char *rate = strstr(type_layer[i], "do") + 2;

				if(!strcmp(option, "train")){
					mask = ((double)rand() / RAND_MAX <= atof(rate));
				}
				else
				if(!strcmp(option, "test")){
					mask = atof(rate);
				}
			}
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double &neuron = this->neuron[0][i][h][j][k][l];

					if(strstr(type_layer[i], "ht")){
						neuron = 2 / (1 + exp(-2 * neuron)) - 1;
					}
					else
					if(strstr(type_layer[i], "ls")){
						neuron = 1 / (1 + exp(-neuron));
					}
					else{
						neuron *= (neuron > 0);
					}

					// dropout
					neuron *= mask;
				}
			}
		}
	}
	else
	if(type_layer[i][0] == 'L'){
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double &neuron = this->neuron[0][i][h][j][k][l];

					if(strstr(type_layer[i], "ce")){
						if(strstr(type_layer[i], "sm")){
							// neuron = neuron;
						}
						else{
							neuron = 1 / (1 + exp(-neuron));
						}
					}
					else
					if(strstr(type_layer[i], "mse")){
						if(strstr(type_layer[i], "ht")){
							neuron = 2 / (1 + exp(-2 * neuron)) - 1;
						}
						else
						if(strstr(type_layer[i], "ia")){
							// neuron = neuron;
						}
						else{
							neuron = 1 / (1 + exp(-neuron));
						}
					}
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Adjust_Parameter(int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	if(type_layer[i][0] == 'C' || type_layer[i][0] == 'L'){
		if(strstr(type_layer[i], "bn")){
			Batch_Normalization_Adjust_Parameter(layer_index, map_index);
		}

		for(int h = 0;h < batch_size;h++){
			double ***derivative	= this->derivative[0][i][h];
			double ***lower_neuron	= this->neuron[0][i - 1][h];
		
			for(int m = 0;m < number_maps[i - 1];m++){
				for(int n = 0;n < kernel_height[i];n++){
					for(int o = 0;o < kernel_width[i];o++){
						double sum = 0;

						for(int k = 0;k < map_height[i];k++){
							for(int l = 0;l < map_width[i];l++){
								int index[2] = {k * stride_height[i] + n, l * stride_width[i] + o};

								if(index[0] < map_height[i - 1] && index[1] < map_width[i - 1]){
									sum += derivative[j][k][l] * lower_neuron[m][index[0]][index[1]];
								}
							}
						}
						weight[i][j][m][n][o] -= sum;
					}
				}
			}

			double sum = 0;

			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					sum += derivative[j][k][l];
				}
			}
			weight[i][j][number_maps[i - 1]][0][0] -= sum;
		}
	}
}
void Convolutional_Neural_Networks::Backpropagate(int layer_index, int map_index){
	if(layer_index == number_layers - 1){
		return;
	}

	int i = layer_index;
	int j = map_index;

	if(type_layer[i + 1][0] == 'C' || type_layer[i + 1][0] == 'L'){
		for(int h = 0;h < batch_size;h++){
			double ***derivative		= this->derivative[0][i][h];
			double ***upper_derivative	= this->derivative[0][i + 1][h];

			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					int ks		 = k / stride_height[i + 1];
					int ls		 = l / stride_width[i + 1];
					int index[2] = {ks - (kernel_height[i + 1] - 1), ls - (kernel_width[i + 1] - 1)};

					double sum = 0;

					if(index[0] < 0) index[0] = 0;
					if(index[1] < 0) index[1] = 0;

					for(int m = 0;m < number_maps[i + 1];m++){
						for(int n = index[0];n < map_height[i + 1] && n <= ks;n++){
							for(int o = index[1];o < map_width[i + 1] && o <= ls;o++){
								sum += upper_derivative[m][n][o] * weight[i + 1][m][j][abs(ks - n)][abs(ls - o)];
							}
						}
					}
					derivative[j][k][l] = sum;
				}
			}
		}
	}
	else
	if(type_layer[i + 1][0] == 'P'){
		if(strstr(type_layer[i + 1], "pad")){
			int margin[] = {(map_height[i + 1] - map_height[i]) / 2, (map_width[i + 1] - map_width[i]) / 2};

			for(int h = 0;h < batch_size;h++){
				double **derivative			= this->derivative[0][i][h][j];
				double **upper_derivative	= this->derivative[0][i + 1][h][j];

				for(int k = 0;k < map_height[i];k++){
					for(int l = 0;l < map_width[i];l++){
						derivative[k][l] = upper_derivative[margin[0] + k][margin[1] + l];
					}
				}
			}
		}
		else{
			int stride[] = {map_height[i] / map_height[i + 1], map_width[i] / map_width[i + 1]};

			for(int h = 0;h < batch_size;h++){
				double **derivative			= this->derivative[0][i][h][j];
				double **upper_derivative	= this->derivative[0][i + 1][h][j];

				for(int k = 0;k < map_height[i];k++){
					for(int l = 0;l < map_width[i];l++){
						derivative[k][l] = upper_derivative[k / stride[0]][l / stride[1]];
					}
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Differentiate(int layer_index, int map_index, double learning_rate, double **target_output){
	int i = layer_index;
	int j = map_index;

	if(type_layer[i][0] == 'C'){
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double &derivative	= this->derivative[0][i][h][j][k][l];
					double &neuron		= this->neuron[0][i][h][j][k][l];

					if(strstr(type_layer[i], "ht")){
						derivative *= (1 - neuron) * (1 + neuron);
					}
					else
					if(strstr(type_layer[i], "ls")){
						derivative *= (1 - neuron) * neuron;
					}
					else{
						derivative *= (neuron > 0);
					}
				}
			}
		}

		if(strstr(type_layer[i], "bn")){
			Batch_Normalization_Differentiate(layer_index, map_index);
		}
	}
	else
	if(type_layer[i][0] == 'L'){
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double &derivative	= this->derivative[0][i][h][j][k][l];
					double &neuron		= this->neuron[0][i][h][j][k][l];

					derivative = learning_rate * (neuron - target_output[h][j]);

					if(strstr(type_layer[i], "ce")){
						if(strstr(type_layer[i], "sm")){
							// derivative = derivative;
						}
						else{
							// derivative = derivative;
						}
					}
					else
					if(strstr(type_layer[i], "mse")){
						if(strstr(type_layer[i], "ht")){
							derivative *= (1 - neuron) * (1 + neuron);
						}
						else
						if(strstr(type_layer[i], "ia")){
							// derivative *= 1;
						}
						else{
							derivative *= (1 - neuron) * neuron;
						}
					}
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Feedforward(int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	if(type_layer[i][0] == 'C' || type_layer[i][0] == 'L'){
		for(int h = 0;h < batch_size;h++){
			double ***lower_neuron	= this->neuron[0][i - 1][h];
			double ***neuron		= this->neuron[0][i][h];

			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double sum = 0;

					for(int m = 0;m < number_maps[i - 1];m++){
						for(int n = 0;n < kernel_height[i];n++){
							for(int o = 0;o < kernel_width[i];o++){
								int index[2] = {k * stride_height[i] + n, l * stride_width[i] + o};

								if(index[0] < map_height[i - 1] && index[1] < map_width[i - 1]){
									sum += lower_neuron[m][index[0]][index[1]] * weight[i][j][m][n][o];
								}
							}
						}
					}
					neuron[j][k][l] = sum + weight[i][j][number_maps[i - 1]][0][0];
				}
			}
		}
	}
	else
	if(type_layer[i][0] == 'P'){
		if(strstr(type_layer[i], "pad")){
			int margin[] = {(map_height[i] - map_height[i - 1]) / 2, (map_width[i] - map_width[i - 1]) / 2};

			for(int h = 0;h < batch_size;h++){
				double **lower_neuron	= this->neuron[0][i - 1][h][j];
				double **neuron			= this->neuron[0][i][h][j];

				for(int k = 0;k < map_height[i];k++){
					for(int l = 0;l < map_width[i];l++){
						neuron[k][l] = 0;
					}
				}
				for(int k = 0;k < map_height[i - 1];k++){
					for(int l = 0;l < map_width[i - 1];l++){
						neuron[margin[0] + k][margin[1] + l] = lower_neuron[k][l];
					}
				}
			}
		}
		else{
			int stride[] = {map_height[i - 1] / map_height[i], map_width[i - 1] / map_width[i]};

			for(int h = 0;h < batch_size;h++){
				double **lower_neuron	= this->neuron[0][i - 1][h][j];
				double **neuron			= this->neuron[0][i][h][j];

				for(int k = 0;k < map_height[i];k++){
					for(int l = 0;l < map_width[i];l++){
						if(strstr(type_layer[i], "avg")){
							double sum = 0;
						
							for(int m = 0;m < stride[0];m++){
								for(int n = 0;n < stride[1];n++){
									sum += lower_neuron[k * stride[0] + m][l * stride[1] + n];
								}
							}
							neuron[k][l] = sum / (stride[0] * stride[1]);
						}
						else
						if(strstr(type_layer[i], "max")){
							double max = -1;
						
							for(int m = 0;m < stride[0];m++){
								for(int n = 0;n < stride[1];n++){
									if(max < lower_neuron[k * stride[0] + m][l * stride[1] + n]){
										max = lower_neuron[k * stride[0] + m][l * stride[1] + n];
									}
								}
							}
							neuron[k][l] = max;
						}
					}
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Softmax(int layer_index){
	int i = layer_index;

	if(strstr(type_layer[i], "sm")){
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					double max = 0;
					double sum = 0;

					double ***neuron = this->neuron[0][i][h];

					for(int j = 0;j < number_maps[i];j++){
						if(max < neuron[j][k][l]){
							max = neuron[j][k][l];
						}
					}
					for(int j = 0;j < number_maps[i];j++){
						neuron[j][k][l] = exp(neuron[j][k][l] - max);
						sum += neuron[j][k][l];
					}
					for(int j = 0;j < number_maps[i];j++){
						neuron[j][k][l] /= sum;
					}
				}
			}
		}
	}
}

void Convolutional_Neural_Networks::Batch_Normalization_Activate(char option[], int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	double gamma		 = this->gamma[i][j];
	double beta			 = this->beta[i][j];
	double &mean		 = this->mean[i][j];
	double &variance	 = this->variance[i][j];
	double &sum_mean	 = this->sum_mean[i][j];
	double &sum_variance = this->sum_variance[i][j];

	double ****neuron			= this->neuron[0][i];
	double ****neuron_batch[2]	= {this->neuron[1][i], this->neuron[2][i]};

	if(!strcmp(option, "train")){
		double sum = 0;

		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					sum += neuron[h][j][k][l];
				}
			}
		}
		sum_mean += (mean = sum / (batch_size * map_height[i] * map_width[i]));
							
		sum = 0;
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					sum += (neuron[h][j][k][l] - mean) * (neuron[h][j][k][l] - mean);
				}
			}
		}
		sum_variance += (variance = sum / (batch_size * map_height[i] * map_width[i]));
			
		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					neuron_batch[0][h][j][k][l] = (neuron[h][j][k][l] - mean) / sqrt(variance + epsilon);
					neuron_batch[1][h][j][k][l] = neuron[h][j][k][l];

					neuron[h][j][k][l] = gamma * neuron_batch[0][h][j][k][l] + beta;
				}
			}
		}
	}
	else
	if(!strcmp(option, "test")){
		double stdv = sqrt(variance + epsilon);

		for(int h = 0;h < batch_size;h++){
			for(int k = 0;k < map_height[i];k++){
				for(int l = 0;l < map_width[i];l++){
					neuron[h][j][k][l] = gamma / stdv * neuron[h][j][k][l] + (beta - gamma * mean / stdv);
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Batch_Normalization_Adjust_Parameter(int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	double sum = 0;

	double ****derivative_batch	= this->derivative[2][i];
	double ****neuron_batch		= this->neuron[1][i];
		
	for(int h = 0;h < batch_size;h++){
		for(int k = 0;k < map_height[i];k++){
			for(int l = 0;l < map_width[i];l++){
				sum += derivative_batch[h][j][k][l] * neuron_batch[h][j][k][l];
			}
		}
	}
	gamma[i][j] -= sum;
						
	sum = 0;
	for(int h = 0;h < batch_size;h++){
		for(int k = 0;k < map_height[i];k++){
			for(int l = 0;l < map_width[i];l++){
				sum += derivative_batch[h][j][k][l];
			}
		}
	}
	beta[i][j] -= sum;
}
void Convolutional_Neural_Networks::Batch_Normalization_Differentiate(int layer_index, int map_index){
	int i = layer_index;
	int j = map_index;

	double derivative_mean;
	double derivative_variance;
	double sum = 0;

	double gamma	= this->gamma[i][j];
	double beta		= this->beta[i][j];
	double mean		= this->mean[i][j];
	double variance	= this->variance[i][j];

	double ****derivative			= this->derivative[0][i];
	double ****derivative_batch[2]	= {this->derivative[1][i], this->derivative[2][i]};
	double ****neuron_batch[2]		= {this->neuron[1][i], this->neuron[2][i]};
		
	for(int h = 0;h < batch_size;h++){
		for(int k = 0;k < map_height[i];k++){
			for(int l = 0;l < map_width[i];l++){
				derivative_batch[0][h][j][k][l] = derivative[h][j][k][l] * gamma;
				sum += derivative_batch[0][h][j][k][l] * (neuron_batch[1][h][j][k][l] - mean);
			}
		}
	}
	derivative_variance = sum * (-0.5) * pow(variance + epsilon, -1.5);
				
	sum = 0;
	for(int h = 0;h < batch_size;h++){
		for(int k = 0;k < map_height[i];k++){
			for(int l = 0;l < map_width[i];l++){
				sum += derivative_batch[0][h][j][k][l];
			}
		}
	}
	derivative_mean = -sum / sqrt(variance + epsilon);
		
	for(int h = 0;h < batch_size;h++){
		for(int k = 0;k < map_height[i];k++){
			for(int l = 0;l < map_width[i];l++){
				derivative_batch[1][h][j][k][l] = derivative[h][j][k][l];

				derivative[h][j][k][l] = derivative_batch[0][h][j][k][l] / sqrt(variance + epsilon) + derivative_variance * 2 * (neuron_batch[1][h][j][k][l] - mean) / (batch_size * map_height[i] * map_width[i]) + derivative_mean / (batch_size * map_height[i] * map_width[i]);
			}
		}
	}
}

void Convolutional_Neural_Networks::Resize_Memory(int batch_size){
	if(this->batch_size != batch_size){
		for(int g = 0;g < number_memory_types;g++){
			for(int i = 0;i < number_layers;i++){
				if(Access_Memory(g, i)){
					for(int h = 0;h < this->batch_size;h++){
						for(int j = 0;j < number_maps[i];j++){
							for(int k = 0;k < map_height[i];k++){
								delete[] derivative[g][i][h][j][k];
								delete[] neuron[g][i][h][j][k];
							}
							delete[] derivative[g][i][h][j];
							delete[] neuron[g][i][h][j];
						}
						delete[] derivative[g][i][h];
						delete[] neuron[g][i][h];
					}
					derivative[g][i] = (double****)realloc(derivative[g][i], sizeof(double***) * batch_size);
					neuron[g][i]	 = (double****)realloc(neuron[g][i],	 sizeof(double***) * batch_size);

					for(int h = 0;h < batch_size;h++){
						derivative[g][i][h] = new double**[number_maps[i]];
						neuron[g][i][h]		= new double**[number_maps[i]];

						for(int j = 0;j < number_maps[i];j++){
							derivative[g][i][h][j]	= new double*[map_height[i]];
							neuron[g][i][h][j]		= new double*[map_height[i]];

							for(int k = 0;k < map_height[i];k++){
								derivative[g][i][h][j][k]	= new double[map_width[i]];
								neuron[g][i][h][j][k]		= new double[map_width[i]];
							}
						}
					}
				}
			}
		}
		this->batch_size = batch_size;
	}
}

bool Convolutional_Neural_Networks::Access_Memory(int type_index, int layer_index){
	int g = type_index;
	int i = layer_index;

	return (g == 0 || strstr(type_layer[i], "bn"));
}

Convolutional_Neural_Networks::Convolutional_Neural_Networks(char **type_layer, int number_layers, int map_width[], int map_height[], int number_maps[]){
	this->kernel_width	= new int[number_layers];
	this->kernel_height	= new int[number_layers];
	this->map_width		= new int[number_layers];
	this->map_height	= new int[number_layers];
	this->number_layers	= number_layers;
	this->number_maps	= new int[number_layers];
	this->stride_width	= new int[number_layers];
	this->stride_height = new int[number_layers];
	this->type_layer	= new char*[number_layers];

	batch_size			= 1;
	number_memory_types	= 3;

	for(int i = 0;i < number_layers;i++){
		this->type_layer[i]	 = new char[strlen(type_layer[i]) + 1];
		strcpy(this->type_layer[i], type_layer[i]);
		this->number_maps[i] = number_maps[i];
		this->map_width[i]	 = (map_width == 0) ? (1):(map_width[i]);
		this->map_height[i]	 = (map_height == 0) ? (1):(map_height[i]);

		if(strstr(type_layer[i], "ks")){
			char *kernel_size = strstr(type_layer[i], "ks");

			kernel_width[i] = atoi(kernel_size + 2);
			kernel_size = strstr(kernel_size, ",");
			kernel_height[i] = (kernel_size && atoi(kernel_size + 1) > 0) ? (atoi(kernel_size + 1)):(kernel_width[i]);
		}
		else{
			kernel_width[i]	 = (i == 0 || type_layer[i][0] == 'P') ? (0):(this->map_width[i - 1] - this->map_width[i] + 1);
			kernel_height[i] = (i == 0 || type_layer[i][0] == 'P') ? (0):(this->map_height[i - 1] - this->map_height[i] + 1);
		}

		if(strstr(type_layer[i], "st")){
			char *stride = strstr(type_layer[i], "st");

			stride_width[i] = atoi(stride + 2);
			stride = strstr(stride, ",");
			stride_height[i] = (stride && atoi(stride + 1) > 0) ? (atoi(stride + 1)):(stride_width[i]);
		}
		else{
			stride_width[i]	 = 1;
			stride_height[i] = 1;
		}
	}

	gamma		 = new double*[number_layers];
	beta		 = new double*[number_layers];
	mean		 = new double*[number_layers];
	variance	 = new double*[number_layers];
	sum_mean	 = new double*[number_layers];
	sum_variance = new double*[number_layers];

	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			gamma[i]		= new double[number_maps[i]];
			beta[i]			= new double[number_maps[i]];
			mean[i]			= new double[number_maps[i]];
			variance[i]		= new double[number_maps[i]];
			sum_mean[i]		= new double[number_maps[i]];
			sum_variance[i]	= new double[number_maps[i]];
		}
	}

	derivative	= new double*****[number_memory_types];
	neuron		= new double*****[number_memory_types];

	for(int g = 0;g < number_memory_types;g++){
		derivative[g]	= new double****[number_layers];
		neuron[g]		= new double****[number_layers];

		for(int i = 0;i < number_layers;i++){
			if(Access_Memory(g, i)){
				derivative[g][i] = new double***[batch_size];
				neuron[g][i]	 = new double***[batch_size];

				for(int h = 0;h < batch_size;h++){
					derivative[g][i][h]	= new double**[number_maps[i]];
					neuron[g][i][h]		= new double**[number_maps[i]];

					for(int j = 0;j < number_maps[i];j++){
						derivative[g][i][h][j]	= new double*[map_height[i]];
						neuron[g][i][h][j]		= new double*[map_height[i]];

						for(int k = 0;k < map_height[i];k++){
							derivative[g][i][h][j][k]	= new double[map_width[i]];
							neuron[g][i][h][j][k]		= new double[map_width[i]];
						}
					}
				}
			}
		}
	}

	weight = new double****[number_layers];

	for(int i = 0;i < number_layers;i++){
		if(kernel_width[i] > 0){
			weight[i] = new double***[number_maps[i]];

			for(int j = 0;j < number_maps[i];j++){
				weight[i][j] = new double**[number_maps[i - 1] + 1];

				for(int k = 0;k < number_maps[i - 1] + 1;k++){
					weight[i][j][k] = new double*[kernel_height[i]];

					for(int l = 0;l < kernel_height[i];l++){
						weight[i][j][k][l] = new double[kernel_width[i]];
					}
				}
			}
		}
	}
}
Convolutional_Neural_Networks::~Convolutional_Neural_Networks(){
	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			delete[] gamma[i];
			delete[] beta[i];
			delete[] mean[i];
			delete[] variance[i];
			delete[] sum_mean[i];
			delete[] sum_variance[i];
		}
	}
	delete[] gamma;
	delete[] beta;
	delete[] mean;
	delete[] variance;
	delete[] sum_mean;
	delete[] sum_variance;

	for(int g = 0;g < number_memory_types;g++){
		for(int i = 0;i < number_layers;i++){
			if(Access_Memory(g, i)){
				for(int h = 0;h < batch_size;h++){
					for(int j = 0;j < number_maps[i];j++){
						for(int k = 0;k < map_height[i];k++){
							delete[] derivative[g][i][h][j][k];
							delete[] neuron[g][i][h][j][k];
						}
						delete[] derivative[g][i][h][j];
						delete[] neuron[g][i][h][j];
					}
					delete[] derivative[g][i][h];
					delete[] neuron[g][i][h];
				}
				delete[] derivative[g][i];
				delete[] neuron[g][i];
			}
		}
		delete[] derivative[g];
		delete[] neuron[g];
	}
	delete[] derivative;
	delete[] neuron;

	for(int i = 0;i < number_layers;i++){
		if(kernel_width[i] > 0){
			for(int j = 0;j < number_maps[i];j++){
				for(int k = 0;k < number_maps[i - 1] + 1;k++){
					for(int l = 0;l < kernel_height[i];l++){
						delete[] weight[i][j][k][l];
					}
					delete[] weight[i][j][k];
				}
				delete[] weight[i][j];
			}
			delete[] weight[i];
		}
	}
	delete[] weight;

	for(int i = 0;i < number_layers;i++){
		delete[] type_layer[i];
	}
	delete[] type_layer;

	delete[] kernel_width;
	delete[] kernel_height;
	delete[] map_width;
	delete[] map_height;
	delete[] number_maps;
	delete[] stride_width;
	delete[] stride_height;
}

void Convolutional_Neural_Networks::Initialize_Parameter(int seed, double scale, double shift){
	srand(seed);

	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			for(int j = 0;j < number_maps[i];j++){
				gamma[i][j]	= 1;
				beta[i][j]	= 0;
			}
		}
		if(kernel_width[i] > 0){
			for(int j = 0;j < number_maps[i];j++){
				for(int k = 0;k < number_maps[i - 1] + 1;k++){
					for(int l = 0;l < kernel_height[i];l++){
						for(int m = 0;m < kernel_width[i];m++){
							weight[i][j][k][l][m] = scale * rand() / RAND_MAX + shift;
						}
					}
				}
			}
		}
	}
}
void Convolutional_Neural_Networks::Load_Parameter(char path[]){
	FILE *file = fopen(path, "rt");

	if(file){
		fscanf(file, "%lf", &epsilon);

		for(int i = 0;i < number_layers;i++){
			if(strstr(type_layer[i], "bn")){
				for(int j = 0;j < number_maps[i];j++) fscanf(file, "%lf", &gamma[i][j]);
				for(int j = 0;j < number_maps[i];j++) fscanf(file, "%lf", &beta[i][j]);
				for(int j = 0;j < number_maps[i];j++) fscanf(file, "%lf", &mean[i][j]);
				for(int j = 0;j < number_maps[i];j++) fscanf(file, "%lf", &variance[i][j]);
			}
			if(kernel_width[i] > 0){
				for(int j = 0;j < number_maps[i];j++){
					for(int k = 0;k < number_maps[i - 1] + 1;k++){
						for(int l = 0;l < kernel_height[i];l++){
							for(int m = 0;m < kernel_width[i];m++){
								fscanf(file, "%lf", &weight[i][j][k][l][m]);
							}
						}
					}
				}
			}
		}
		fclose(file);
	}
	else{
		fprintf(stderr, "[Load_Parameter], %s not found\n", path);
	}
}
void Convolutional_Neural_Networks::Save_Parameter(char path[]){
	FILE *file = fopen(path, "wt");

	fprintf(file, "%f\n", epsilon);

	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			for(int j = 0;j < number_maps[i];j++) fprintf(file, "%f\n", gamma[i][j]);
			for(int j = 0;j < number_maps[i];j++) fprintf(file, "%f\n", beta[i][j]);
			for(int j = 0;j < number_maps[i];j++) fprintf(file, "%f\n", mean[i][j]);
			for(int j = 0;j < number_maps[i];j++) fprintf(file, "%f\n", variance[i][j]);
		}
		if(kernel_width[i] > 0){
			for(int j = 0;j < number_maps[i];j++){
				for(int k = 0;k < number_maps[i - 1] + 1;k++){
					for(int l = 0;l < kernel_height[i];l++){
						for(int m = 0;m < kernel_width[i];m++){
							fprintf(file, "%f\n", weight[i][j][k][l][m]);
						}
					}
				}
			}
		}
	}
	fclose(file);
}
void Convolutional_Neural_Networks::Test(double input[], double output[]){
	Resize_Memory(1);

	#pragma omp parallel for
	for(int h = 0;h < number_maps[0] * map_height[0] * map_width[0];h++){
		int j = (h / (map_height[0] * map_width[0]));
		int k = (h % (map_height[0] * map_width[0])) / map_width[0];
		int l = (h % (map_height[0] * map_width[0])) % map_width[0];

		neuron[0][0][0][j][k][l] = input[h];
	}

	for(int i = 1;i < number_layers;i++){
		#pragma omp parallel for
		for(int j = 0;j < number_maps[i];j++){
			Feedforward	(i, j);
			Activate	("test", i, j);
		}
		Softmax(i);
	}
	for(int i = number_layers - 1, j = 0;j < number_maps[i];j++){
		output[j] = neuron[0][i][0][j][0][0];
	}
}

double Convolutional_Neural_Networks::Train(int batch_size, int number_training, double epsilon, double learning_rate, double **input, double **target_output){
	int *index = new int[number_training];

	double loss = 0;

	double **target_output_batch = new double*[batch_size];

	for(int i = 0;i < number_training;i++){
		index[i] = i;
	}
	for(int i = 0;i < number_training;i++){
		int j = rand() % number_training;
		int t = index[i];

		index[i] = index[j];
		index[j] = t;
	}

	for(int h = 0;h < batch_size;h++){
		target_output_batch[h] = new double[number_maps[number_layers - 1]];
	}
	Resize_Memory(batch_size);

	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			for(int j = 0;j < number_maps[i];j++){
				sum_mean[i][j]		= 0;
				sum_variance[i][j]	= 0;
			}
		}
	}
	this->epsilon = epsilon;

	for(int g = 0, h = 0;g < number_training;g++){
		#pragma omp parallel for
		for(int i = 0;i < number_maps[0] * map_height[0] * map_width[0];i++){
			int j = (i / (map_height[0] * map_width[0]));
			int k = (i % (map_height[0] * map_width[0])) / map_width[0];
			int l = (i % (map_height[0] * map_width[0])) % map_width[0];

			neuron[0][0][h][j][k][l] = input[index[g]][i];
		}
		for(int j = 0;j < number_maps[number_layers - 1];j++){
			target_output_batch[h][j] = target_output[index[g]][j];
		}

		if(++h == batch_size){
			h = 0;

			for(int i = 1;i < number_layers;i++){
				#pragma omp parallel for
				for(int j = 0;j < number_maps[i];j++){
					Feedforward	(i, j);
					Activate	("train", i, j);
				}
				Softmax(i);
			}

			for(int i = number_layers - 1;i > 0;i--){
				#pragma omp parallel for
				for(int j = 0;j < number_maps[i];j++){
					Backpropagate(i, j);
					Differentiate(i, j, learning_rate, target_output_batch);
				}
			}
			for(int i = number_layers - 1;i > 0;i--){
				#pragma omp parallel for
				for(int j = 0;j < number_maps[i];j++){
					Adjust_Parameter(i, j);
				}
			}

			for(int h = 0;h < batch_size;h++){
				for(int i = number_layers - 1, j = 0;j < number_maps[i];j++){
					if(strstr(type_layer[i], "ce")){
						loss -= target_output_batch[h][j] * log(neuron[0][i][h][j][0][0] + 0.000001) + (1 - target_output_batch[h][j]) * log(1 - neuron[0][i][h][j][0][0] + 0.000001);
					}
					if(strstr(type_layer[i], "mse")){
						loss += 0.5 * (neuron[0][i][h][j][0][0] - target_output_batch[h][j]) * (neuron[0][i][h][j][0][0] - target_output_batch[h][j]);
					}
				}
			}
		}
	}

	for(int i = 0;i < number_layers;i++){
		if(strstr(type_layer[i], "bn")){
			for(int j = 0;j < number_maps[i];j++){
				mean[i][j]		= sum_mean[i][j] / (number_training / batch_size);
				variance[i][j]	= ((double)batch_size / (batch_size - 1)) * sum_variance[i][j] / (number_training / batch_size);
			}
		}
	}

	for(int h = 0;h < batch_size;h++){
		delete[] target_output_batch[h];
	}
	delete[] index;
	delete[] target_output_batch;

	return loss / number_training;
}
