# encoding = utf-8

import torch

# GPU device setting
device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')

# model parameter setting
batch_size = 128
max_len = 256
d_model = 512
n_layer = 6
n_head = 8
ffn_hidden = 2048
drop_prob = 0.1

# optimizer parameter setting
init_lr = 1e-5
factor = 0.9
adam_eps = 5e-9
patience = 10
warmup = 100
epoch = 100
clip = 1.0
weight_decay = 5e-4
inf = float('inf')