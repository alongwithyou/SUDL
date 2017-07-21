// conv_layer.cpp

#include "conv_layer.h"
#include "pooling_layer.h"
#include "full_conn_layer.h"

namespace sub_dl {


ConvLayer::ConvLayer(int input_dim, int output_dim, int kernel_x_dim, int kernel_y_dim, int feature_x_dim, int feature_y_dim) {

    _input_dim = input_dim;
    _output_dim = output_dim;
    _kernel_x_dim = kernel_x_dim;
    _kernel_y_dim = kernel_y_dim;
    _feature_x_dim = feature_x_dim;
    _feature_y_dim = feature_y_dim;

    _feature_x_dim = feature_x_dim;
    _feature_y_dim = feature_y_dim;

    _conv_kernels.resize(_input_dim, _output_dim);
    _delta_conv_kernels.resize(_input_dim, _output_dim);
    for (int i = 0; i < _input_dim;i ++) {
        for (int j = 0; j < _output_dim; j++) {    
            matrix_double kernel(_kernel_x_dim, _kernel_y_dim);
            kernel.assign_val();
            _conv_kernels[i][j] = kernel;
            _delta_conv_kernels[i][j].resize(_kernel_x_dim, _kernel_y_dim);
        }
    }
    _conv_bias.resize(1, _output_dim);
    _conv_bias.assign_val();
    _delta_conv_bias.resize(1, _output_dim);
    _conn_map.resize(_input_dim, _output_dim);
    _conn_map.resize(1);
    _type = CONV;

}

void ConvLayer::display() {
    std::cout << "----------conv layer----------" << std::endl;
    std::cout << "-----------feature map------" << std::endl;
    for (int i = 0; i < _output_dim;i ++) {
        std::cout << "feature map " << i << std::endl;
        _data[i]._display();
    }
    std::cout << "-----------kernel------" << std::endl;
    for (int i = 0; i < _input_dim;i ++) {
        for (int j = 0; j < _output_dim;j ++) {
            std::cout << "kernel " << i << " " << j << std::endl;
            if (_conn_map[i][j]) {
                _conv_kernels[i][j]._display();
            } else {
                std::cout << "NULL" << std::endl;
            }
        }
    }
    std::cout << "-----------bias-------" << std::endl;
    _conv_bias._display();
    std::cout << "---error--" << std::endl;
    if (_errors.size() > 0) {
        for (int i = 0; i < _output_dim; i++) {
            _errors[i]._display();
        }
    }
    std::cout << "----------conv layer end----------" << std::endl;
}

void ConvLayer::_forward(Layer* pre_layer) {
    // save the pre layer pointer for backward weight update
    _pre_layer = pre_layer;
    std::vector<matrix_double>().swap(_data);
    for (int i = 0; i < _output_dim; i++) {
        matrix_double feature_map(_feature_x_dim, _feature_y_dim);
        for (int j = 0; j < _input_dim; j++) {
            if (_conn_map[j][i]) {
                feature_map = feature_map + pre_layer->_data[j].conv(_conv_kernels[j][i]);
            }
        }
        _data.push_back(sigmoid_m(feature_map + _conv_bias[0][i]));
    }
}

void ConvLayer::_backward(Layer* nxt_layer) {
    std::vector<matrix_double>().swap(_errors);
    if (nxt_layer->_type == POOL) {
        const PoolingLayer* pooling_layer = (PoolingLayer*)(nxt_layer);
        int delta_size = (pooling_layer->_pooling_x_dim) * pooling_layer->_pooling_y_dim;
        for (int i = 0; i < _output_dim; i++) {
            matrix_double up_delta = pooling_layer->_errors[i]
                .up_sample(pooling_layer->_pooling_x_dim, 
                pooling_layer->_pooling_y_dim);
            matrix_double error = up_delta.dot_mul(sigmoid_m_diff(_data[i]))
                * (pooling_layer->_pooling_weights[0][i] / delta_size);
            _errors.push_back(error);
        }
    } else if (nxt_layer->_type == FULL_CONN) {
        const FullConnLayer* full_conn_layer = (FullConnLayer*)(nxt_layer);

        full_conn_layer->_errors[0]._display("full error");
        full_conn_layer->_full_conn_weights._display("full weights");
        for (int i = 0; i < _output_dim; i++) {
            _data[i]._display("data");
        }
        for (int i = 0; i < _input_dim; i++) {
            _pre_layer->_data[i]._display("input");
        }

        for (int i = 0; i < _output_dim; i++) {
            matrix_double error(_feature_x_dim, _feature_y_dim);
            for (int u = 0; u < error._x_dim; u++) {
                for (int v = 0; v < error._y_dim; v++) {
                    for (int t = 0; t < full_conn_layer->_output_dim; t++) {
                        error[u][v] += full_conn_layer->_errors[0][0][t] * 
                            full_conn_layer->_full_conn_weights[i * (error._x_dim * error._y_dim) 
                                + error._y_dim * u + v][t];
                    }
                    error[u][v] *= sigmoid_diff(_data[i][u][v]);
                }
            }
            error._display("error");
            _errors.push_back(error);
        }

    }
    for (int i = 0; i < _output_dim; i++) {
        // update kernel weights
        for (int j = 0; j < _input_dim; j++) {
            if (_conn_map[j][i]) {
                _delta_conv_kernels[j][i] = (_pre_layer->_data[j]
                    .conv(_errors[i].rotate_180()))
                    .rotate_180();
            }
        }
        // update kernel bias
        _delta_conv_bias[0][i] = _errors[i].sum();
    }
}

void ConvLayer::_update_gradient(int opt_type, double learning_rate) {
    if (opt_type == SGD) {
        _conv_kernels.add(_delta_conv_kernels * learning_rate);
        _conv_bias.add(_delta_conv_bias * learning_rate);
    }
}

}
