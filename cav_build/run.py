# %% md
# This Python 3 environment comes with many helpful analytics libraries installed
# It is defined by the kaggle/python Docker image: https://github.com/kaggle/docker-python
# For example, here's several helpful packages to load

# import numpy as np  # linear algebra
# import pandas as pd  # data processing, CSV file I/O (e.g. pd.read_csv)

# Input data files are available in the read-only "../input/" directory
# For example, running this (by clicking run or pressing Shift+Enter) will list all files under the input directory

import os
from io import BytesIO
from tqdm import tqdm
import pandas as pd
import math
import numpy as np
import matplotlib.pyplot as plt

import torch
import torch.nn as nn
import torch.nn.functional as F
from torch.utils.data import Dataset
from torch.utils.data import DataLoader
from torch.utils.tensorboard import SummaryWriter

from sklearn.preprocessing import StandardScaler, MaxAbsScaler, Normalizer, RobustScaler, MinMaxScaler
from sklearn.base import BaseEstimator, TransformerMixin
from sklearn.model_selection import train_test_split
from sklearn.metrics import r2_score, accuracy_score, mean_squared_error, mean_absolute_error

from cryptography.fernet import Fernet

# default `log_dir` is "runs" - we'll be more specific here
writer = SummaryWriter('runs/cav_build')

torch.cuda.empty_cache()

class PositionalEncoding(nn.Module):
    """ 标准sinusoidal位置编码 """

    def __init__(self, d_model=6, max_len=16):
        super().__init__()
        pe = torch.zeros(max_len, d_model)
        position = torch.arange(0, max_len, dtype=torch.float).unsqueeze(1)
        div_term = torch.exp(torch.arange(0, d_model, 2).float() * (-math.log(10000.0) / d_model))
        pe[:, 0::2] = torch.sin(position * div_term)
        pe[:, 1::2] = torch.cos(position * div_term)
        self.register_buffer('pe', pe.unsqueeze(0))  # [1, max_len, d_model]

    def forward(self, x):
        return x + self.pe[:, :x.size(1)]

class Attention(nn.Module):
    def __init__(self):
        super().__init__()
        # 定义各位置组的投影矩阵
        self.q_proj = nn.ModuleDict({
            'p0': nn.Linear(6, 6),
            'p1': nn.Linear(6, 6),
            'p2': nn.Linear(6, 6),
            'p3': nn.Linear(6, 6),
            'p4': nn.Linear(6, 6)
        })
        self.k_proj = nn.ModuleDict({
            'p0': nn.Linear(6, 6),
            'p1': nn.Linear(6, 6),
            'p2': nn.Linear(6, 6),
            'p3': nn.Linear(6, 6),
            'p4': nn.Linear(6, 6)
        })
        self.v_proj = nn.ModuleDict({
            'p0': nn.Linear(6, 6),
            'p1': nn.Linear(6, 6),
            'p2': nn.Linear(6, 6),
            'p3': nn.Linear(6, 6),
            'p4': nn.Linear(6, 6)
        })

        self.out_proj = nn.Linear(6, 6)
        self.num_heads = 2
        self.head_dim = 6 // self.num_heads  # 明确头维度

    def _group_project(self, x, proj_dict):
        # 确保输入序列长度是16（位置0-15）
        assert x.size(1) == 16, "输入序列长度必须为16"
        outputs = []
        outputs.append(proj_dict['p0'](x[:, 0:1]))  # 位置0
        outputs.append(proj_dict['p1'](x[:, 1:6]))  # 位置1-5
        outputs.append(proj_dict['p2'](x[:, 6:9]))  # 位置6-8
        outputs.append(proj_dict['p3'](x[:, 9:10]))  # 位置9
        outputs.append(proj_dict['p4'](x[:, 10:16]))  # 位置10-15
        return torch.cat(outputs, dim=1)

    def forward(self, x, mask=None):
        batch_size, seq_len, _ = x.size()

        # 投影到Q/K/V空间
        Q = self._group_project(x, self.q_proj)
        K = self._group_project(x, self.k_proj)
        V = self._group_project(x, self.v_proj)

        # 重塑为多头格式 (batch_size, num_heads, seq_len, head_dim)
        Q = Q.view(batch_size, seq_len, self.num_heads, self.head_dim).transpose(1, 2)
        K = K.view(batch_size, seq_len, self.num_heads, self.head_dim).transpose(1, 2)
        V = V.view(batch_size, seq_len, self.num_heads, self.head_dim).transpose(1, 2)

        # 计算注意力分数
        attn = (Q @ K.transpose(-2, -1)) / math.sqrt(self.head_dim)

        # 应用掩码（若提供）
        # if mask is not None:
        #     attn = attn.masked_fill(mask == 0, float('-inf'))

        attn = torch.softmax(attn, dim=-1)
        x = (attn @ V).transpose(1, 2).contiguous().view(batch_size, seq_len, -1)

        return self.out_proj(x)

class FFN(nn.Module):
    def __init__(self,dim_feedforward,dropout):
        super().__init__()

        self.ffn = nn.ModuleDict({
            's0': nn.Sequential(nn.Linear(6, dim_feedforward), nn.ReLU(),  nn.Dropout(dropout),nn.Linear(dim_feedforward, 6)),
            's1': nn.Sequential(nn.Linear(6, dim_feedforward),nn.ReLU(),  nn.Dropout(dropout),nn.Linear(dim_feedforward, 6)),
            's2': nn.Sequential(nn.Linear(6, dim_feedforward), nn.ReLU(),  nn.Dropout(dropout),nn.Linear(dim_feedforward, 6)),
            's3': nn.Sequential(nn.Linear(6, dim_feedforward), nn.ReLU(), nn.Dropout(dropout), nn.Linear(dim_feedforward, 6)),
            's4': nn.Sequential(nn.Linear(6, dim_feedforward), nn.ReLU(),  nn.Dropout(dropout),nn.Linear(dim_feedforward, 6))
        })

    def forward(self, x):
        # 分割输入到不同位置组
        parts = [
            self.ffn['s0'](x[:, 0:1]),
            self.ffn['s1'](x[:, 1:6]),
            self.ffn['s2'](x[:, 6:9]),
            self.ffn['s3'](x[:, 9:10]),
            self.ffn['s4'](x[:, 10:16])
        ]
        return torch.cat(parts, dim=1)

class EncoderBlock(nn.Module):
    def __init__(self, first_block):
        super().__init__()
        # 共享第一个block的组件
        self.attn = first_block.attn
        self.ffn = first_block.ffn
        self.norm1 = first_block.norm1
        self.norm2 = first_block.norm2

    def forward(self, x):
        attn_out = self.attn(x)
        x = self.norm1(x + attn_out)
        ffn_out = self.ffn(x)
        return self.norm2(x + ffn_out)

class MyTransformer(nn.Module):
    def __init__(self, num_layers=3,dim_feedforward=24,dropout=0.5):
        super().__init__()

        self.scaler = nn.BatchNorm1d(81)

        self.register_buffer('a', torch.tensor(93.51615326821938))
        self.register_buffer('b', torch.tensor(42.932214709074216))
        self.register_buffer('c', torch.tensor(11.842307371232991))
        self.register_buffer('d', torch.tensor(3007.8648555597297))
        self.register_buffer('e', torch.tensor(1.1138332387644279))
        self.register_buffer('f', torch.tensor(215.77770473328326))
        # 创建索引掩码
        self.pos_encoder = PositionalEncoding()

        # 第一个block
        self.first_block = nn.ModuleDict({
            'attn': Attention(),
            'ffn': FFN(dim_feedforward=dim_feedforward, dropout=dropout),
            'norm1': nn.LayerNorm(6),
            'norm2': nn.LayerNorm(6)
        })

        # 后续共享block
        self.layers = nn.ModuleList([
            EncoderBlock(self.first_block) for _ in range(num_layers - 1)
        ])

        self.last_block = nn.ModuleDict({

            'ffn': FFN(dim_feedforward=dim_feedforward, dropout=dropout),
            'norm1': nn.LayerNorm(6),
            'norm2': nn.LayerNorm(6)
        })

        self.fc1 = nn.Linear(6, 96)

        self.fc2 = nn.Linear(16 * 6, 23)

        # self.relu= nn.ReLU()

    def forward(self, input):

        # input=input / self.mask

        B = input.shape[0]
        input[:, 0] = input[:, 0] / self.a
        # b_col = {[1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25, 26] + \
        #     [53, 54, 55] + [58, 59, 60] + [63, 64, 65] + [68, 69, 70] + [73, 74, 75] + [78, 79, 80]}
        # b_col = {1, 2, 4, 5, 7, 8, 10, 11, 13, 14, 16, 17, 19, 20, 22, 23, 25, 26, 53, 54, 55, 58, 59, 60, 63, 64, 65, 68, 69, 70, 73, 74, 75,78, 79, 80}
        for idx in [1, 2, 4, 5, 7, 8, 10, 
                11, 13, 14, 16, 17, 19, 20, 
                22, 23, 25, 26, 53, 54, 55, 58, 59, 60, 
                63, 64, 65, 68, 69, 70, 73, 74, 75, 78, 79, 80]:
            input[:, idx] = input[:, idx] / self.b
        # input[:, b_col] = input[:, b_col] / self.b
        input[:, 3:28:3] = input[:, 3:28:3] / self.c
        input[:, 28] = input[:, 28] / self.d
        input[:, 29:51] = input[:, 29:51] / self.e
        input[:, 52:78:5] = input[:, 52:78:5] / self.f

        # 创建目标矩阵并初始化为最后一个值（索引80）
        # last_val = x[:, -1].unsqueeze(-1).unsqueeze(-1)  # (B, 1, 1)
        # x = torch.full((B, 16, 6), fill_value=0,dtype=input.dtype, device=input.device)

        # first_values = x[:, 0]

        # 增加维度并扩展 (B, 16, 6)
        x = input[:, 0].reshape(-1, 1, 1).expand(-1, 16, 6).clone()
        # target = first_values.view(-1, 1, 1).expand(-1, 16, 6).clone()
        # x = torch.full((16, 6), input[0], dtype=input.dtype, device=input.device)

        # x[:,0, 0:6:2] = input[:,0]
        # x[:,0, 0:6:2] = input[:, 0].unsqueeze(-1).expand(-1, 3)
        # x[:,0, 0:6:2] = input[:,28].unsqueeze(-1)
        # x[:,0, 0:6:2] = input[:,28].unsqueeze(-1).expand(-1, 3)
        x[:, 0, 1:6] = input[:, 28].unsqueeze(-1).expand(-1, 5)

        # 第 1-5 行处理
        x[:,1, :] = input[:, 1:7]  # 索引 1-6（6 元素）
        x[:,2, :] = input[:, 7:13] # 索引 7-12
        x[:,3, :] = input[:, 13:19]  # 索引 13-18
        x[:,4, :] = input[:, 19:25]  # 索引 19-24
        x[:,5, :3] = input[:, 25:28]  # 索引 25-27（3 元素）
        # x[:,5, 3:] = input[:, 0].unsqueeze(-1).expand(-1, 3)

        # 第 6-9 行处理
        x[:, 6, :] = input[:, 29:35]  # 索引 29-34
        x[:, 7, :] = input[:, 35:41]  # 索引 35-40
        x[:, 8, :] = input[:, 41:47]  # 索引 41-46
        x[:, 9, :4] = input[:, 47:51]  # 索引 47-50（4 元素）

        # 第 10-15 行处理
        x[:, 10, :5] = input[:, 51:56]  # 索引 51-55
        x[:, 11, :5] = input[:, 56:61]  # 索引 56-60
        x[:, 12, :5] = input[:, 61:66]  # 索引 61-65
        x[:, 13, :5] = input[:, 66:71]  # 索引 66-70
        x[:, 14, :5] = input[:, 71:76]  # 索引 71-75
        x[:, 15, :5] = input[:, 76:81]

        # x[:,10:16,5]=input[:,0]
        x = x.detach()

        x = self.pos_encoder(x)
        x = self._apply_block(x, self.first_block)

        # for layer in self.layers:
        #     x = layer(x)

        x = self.fc2(x.flatten(1))

        # x = self.relu(x)

        output = x.detach().clone()

        output[:, 0] = output[:, 0] * self.d
        output[:, 1:] = output[:, 1:] * self.e

        return output, x
        # return self.fc(x.flatten(1))
        # return self.relu(self.fc(x.flatten(1)))

    def _apply_block(self, x, block):
        attn_out = block['attn'](x)
        x = block['norm1'](x + attn_out)
        ffn_out = block['ffn'](x)
        return block['norm2'](x + ffn_out)

class WeightedMSELoss(nn.Module):
    def __init__(self, weights):
        super(WeightedMSELoss, self).__init__()
        # self.weights = torch.tensor(weights)
        self.weights = weights

    def forward(self, output, target):
        # 计算每个元素的损失
        loss_per_element = (output - target) ** 2
        # 应用权重
        weighted_loss = loss_per_element * self.weights
        # 返回加权损失的平均值
        return weighted_loss.mean()

class TransformerEncoder(nn.Module):
    def __init__(self, d_model=128, nhead=8, num_layers=3, dim_feedforward=64):
        super(TransformerEncoder, self).__init__()
        # 输入线性层，将输入维度映射到 d_model 维度
        # self.input_linear = nn.Linear(input_dim, d_model)
        # 位置编码层
        self.positional_encoding = PositionalEncoding(d_model, max_len=16)
        # 定义 Transformer 编码器层
        encoder_layer = nn.TransformerEncoderLayer(d_model=d_model, nhead=nhead, dim_feedforward=dim_feedforward,
                                                   dropout=0)
        # 堆叠多个编码器层形成完整的编码器
        self.transformer_encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
        # 输出线性层，将 d_model 维度映射到输出维度
        self.output_linear = nn.Linear(d_model, 1)

    def forward(self, x):
        # 通过输入线性层进行维度映射
        # x = self.input_linear(x)
        # 添加位置编码
        x = self.positional_encoding(x)
        # 因为 nn.TransformerEncoder 期望输入形状为 (S, N, E)，这里进行形状调整
        x = x.transpose(0, 1)
        # 通过 Transformer 编码器
        x = self.transformer_encoder(x)
        # 恢复原始形状
        x = x.transpose(0, 1)
        # 取每个样本最后一个时间步的特征
        x = x[:, -1, :]
        # 通过输出线性层得到预测值
        output = self.output_linear(x)
        return output

class IdentityScaler(BaseEstimator, TransformerMixin):
    def fit(self, X, y=None):
        return self

    def transform(self, X):
        return X

    def inverse_transform(self, X):
        return X

class PositionwiseFeedForward(nn.Module):
    def __init__(self, d_model, dff):
        super(PositionwiseFeedForward, self).__init__()
        self.w1 = nn.Linear(d_model, dff)
        self.w2 = nn.Linear(dff, d_model)

    def forward(self, x):
        return self.w2(torch.relu(self.w1(x)))

class LayerNorm(nn.Module):
    def __init__(self, features, eps=1e-6):
        super(LayerNorm, self).__init__()
        self.a_2 = nn.Parameter(torch.ones(features))
        self.b_2 = nn.Parameter(torch.zeros(features))
        self.eps = eps

    def forward(self, x):
        mean = x.mean(-1, keepdim=True)
        std = x.std(-1, keepdim=True)
        # std = 1
        return self.a_2 * (x - mean) / (std + self.eps) + self.b_2

class TransformerEncoderBlock(nn.Module):
    def __init__(self, d_model, num_heads, dff, dropout=0.1):
        super(TransformerEncoderBlock, self).__init__()

        self.mha = nn.MultiheadAttention(d_model, num_heads)
        self.ffn = PositionwiseFeedForward(d_model, dff)
        self.layernorm1 = LayerNorm(d_model)
        self.layernorm2 = LayerNorm(d_model)
        self.dropout1 = nn.Dropout(dropout)
        self.dropout2 = nn.Dropout(dropout)

    def forward(self, x):
        # 多头注意力子层，不需要位置编码和掩码
        attn_output, attn_output_weights = self.mha(x, x, x)
        attn_output = self.dropout1(attn_output)
        out1 = self.layernorm1(x + attn_output)

        # 前馈神经网络子层
        ffn_output = self.ffn(out1)
        ffn_output = self.dropout2(ffn_output)
        return self.layernorm2(out1 + ffn_output)

class FloodDataset(Dataset):
    def __init__(self, features, labels):
        self.features = torch.FloatTensor(features)
        self.labels = torch.FloatTensor(labels)

    def __len__(self):
        return len(self.features)

    def __getitem__(self, idx):
        return self.features[idx], self.labels[idx]

class CustomDataset(Dataset):
    def __init__(self, data, labels):
        """
        初始化CustomDataset实例。

        参数:
        - data: 一个列表或元组，其中每个元素都是一个包含两个数组的元组或列表（即(array1, array2)）。
        - labels: 一个与data中元素一一对应的标签列表。
        """
        self.x = []
        # self.h = []
        for d in data:
            self.x.append(tuple(d))
            # self.h.append(data[i][1])

        self.y = labels

    def __len__(self):
        """
        返回数据集中的样本总数。
        """
        return len(self.y)

    def __getitem__(self, idx):
        """
        根据索引获取一个样本及其标签。

        参数:
        - idx: 样本的索引。

        返回:
        - 一个元组，其中第一个元素是包含两个数组的features（即(array1, array2)），第二个元素是标签。
        """
        # 根据索引获取数据样本，这里假设每个样本是(array1, array2)的形式
        # array1, array2 = self.data[idx][0],self.data[idx][1]
        #
        # # 将两个数组打包成features
        # features = (array1, array2)
        #
        # # 获取对应的标签
        # label = self.labels[idx]

        # 返回features和label
        return self.x[idx], self.y[idx]

class ZaoqiangDataset(Dataset):
    def __init__(self, features, labels):
        self.features = torch.FloatTensor(features)
        self.labels = torch.FloatTensor(labels)

    def __len__(self):
        return len(self.features)

    def __getitem__(self, idx):
        return self.features[idx], self.labels[idx]

def get_positional_encoding(max_pos, d_model):
    pos_encoding = np.zeros((max_pos, d_model))
    for pos in range(max_pos):
        for k in range(d_model // 2):
            pos_encoding[pos, 2 * k] = np.sin(pos / (10000 ** ((2 * k) / d_model)))
            pos_encoding[pos, 2 * k + 1] = np.cos(pos / (10000 ** ((2 * k + 1) / d_model)))
    return pos_encoding

def prepare_data(file_path, batch_size=128):
    # 读取数据
    df = pd.read_csv(file_path)

    # 分离特征和标签
    X = df.iloc[:, :20].values
    y = df.iloc[:, 20].values

    # 0-1标准化
    scaler = StandardScaler()
    X = scaler.fit_transform(X)

    # 划分训练集和测试集
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.1, random_state=42
    )

    # 创建数据加载器
    train_dataset = FloodDataset(X_train, y_train)
    test_dataset = FloodDataset(X_test, y_test)

    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    test_loader = DataLoader(test_dataset, batch_size=batch_size * 4)

    return train_loader, test_loader

def prepare_cav_build_data(file_path, batch_size=128):
    # 读取数据
    df = pd.read_excel(file_path, engine='openpyxl')

    bool_list = []
    for j, column in enumerate(df.columns):
        # 检查列名是否包含“正反循环”
        if "正反循环" in column:
            # 对该列的内容进行检查和修改
            df[column] = df[column].apply(lambda x: 1 if x == 1 else -1)
            bool_list.append(j)

    X = df.iloc[:, :81].values
    y = df.iloc[:, 81:].values

    # Scaler_x = RobustScaler()
    Scaler_x = IdentityScaler()
    # Scaler_x = MinMaxScaler()
    Scaler_y = IdentityScaler()
    # Scaler_y = RobustScaler()
    #
    #
    # std_4 = np.std(X[:, 28])
    mean_4 = np.mean(X[:, 28])
    # X_sc[:, 28] = X[:, 28] / mean_4
    #
    # std_5 = np.std(X[:, 29:51])
    mean_5 = np.mean(X[:, 29:51])

    # y[:, 0] = y[:, 0] / std_4
    y[:, 0] = y[:, 0] / mean_4
    # y[:, 1:] = y[:, 1:] / std_5
    y[:, 1:] = y[:, 1:] / mean_5

    # 划分训练集和测试集
    # X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.15, random_state=0)
    X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=0)

    # 创建数据加载器
    train_dataset = ZaoqiangDataset(X_train, y_train)
    test_dataset = ZaoqiangDataset(X_test, y_test)
    # train_dataset = CustomDataset(X_train, y_train)
    # test_dataset = CustomDataset(X_test, y_test)

    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    test_loader = DataLoader(test_dataset, batch_size=batch_size * 4,shuffle=False)

    return train_loader, test_loader, mean_4, mean_5
    # return train_loader, test_loader, std_4, std_5

def train_model(model, train_loader, test_loader, model_save_path, epochs, optimizer):
    device = torch.device('cuda:0' if torch.cuda.is_available() else 'cpu')
    model = model.to(device)
    criterion = nn.MSELoss()
    # criterion = WeightedMSELoss(torch.tensor([250] + [1] * 22).to(device))
    # criterion = WeightedMSELoss(torch.tensor([0.78] + [0.01] * 22).to(device))

    best_val_loss = float('inf')
    best_epoch = 0

    for epoch in range(epochs):
        model.train()
        running_loss = 0.0
        pbar = tqdm(train_loader, desc=f'Epoch [{epoch + 1}/{epochs}]')
        for batch_x, batch_y in pbar:
        # for batch_x0, batch_x1, batch_x2, batch_y in pbar:
            batch_x, batch_y = batch_x.to(device), batch_y.to(device)

            optimizer.zero_grad()
            _, outputs = model(batch_x)
            # outputs = model(batch_x, batch_h)
            # outputs = model(batch_x0, batch_x1, batch_x2)
            # outputs shape: [batch_size, 1]
            loss = criterion(outputs.squeeze(), batch_y)
            loss.backward()
            optimizer.step()
            running_loss += loss.item()

            # 更新进度条显示的损失值
            pbar.set_postfix({'loss': f'{loss.item():.4f}'})

        model.eval()
        val_loss = 0.0
        mae = 0.0
        total = 0
        with torch.no_grad():
            # for batch_x0,batch_x1,batch_x2, batch_y in test_loader:
            for batch_x, batch_y in test_loader:
                batch_x, batch_y = batch_x.float().to(device), batch_y.float().to(device)

                _,outputs = model(batch_x)
                # 计算MSE和MAE
                val_loss += criterion(outputs.squeeze(), batch_y).item()
                mae += torch.abs(outputs.squeeze() - batch_y).sum().item()
                total += batch_y.size(0)

        avg_val_loss = val_loss / len(test_loader)
        avg_mae = mae / total

        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            best_epoch = epoch

            # torch.save(model.state_dict(), model_save_path)

            torch.save({
                'epoch': epoch,
                'model_state': model.state_dict(),
                'optimizer_state': optimizer.state_dict(),
                # 'loss_history': loss_history,
                # 'scheduler_state': scheduler.state_dict()
            }, model_save_path)

            print(f'Model saved with validation loss: {best_val_loss:.4f}')

        if epoch % 1000 == 0:
            torch.save({
                'epoch': epoch,
                'model_state': model.state_dict(),
                'optimizer_state': optimizer.state_dict(),
                # 'loss_history': loss_history,
                # 'scheduler_state': scheduler.state_dict()
            }, model_save_path.replace('.pt', '_last.pt'))

        print(
            f'Epoch [{epoch + 1}/{epochs}]  Training Loss: {running_loss / len(train_loader):.4f}  Validation MSE: {avg_val_loss:.4f}  Validation MAE: {avg_mae:.4f}')

        writer.add_scalar('Training Loss', running_loss / len(train_loader), epoch)
        writer.add_scalar('Validating Loss', avg_val_loss, epoch)
        writer.add_scalar('Validating MAE', avg_mae, epoch)

    return best_epoch, best_val_loss

def evaluate_model(model, test_loader, device, v_sc, r_sc):
    model.eval()
    predictions = []
    actuals = []

    with torch.no_grad():
        for batch_x, batch_y in test_loader:
            batch_x, batch_y = batch_x.float().to(device), batch_y.float().to(device)
            y, outputs = model(batch_x)

            predictions.extend(y.squeeze().cpu().numpy())
            actuals.extend(batch_y.cpu().numpy())

    predictions = np.array(predictions)
    actuals = np.array(actuals)

    if isinstance(r_sc, (int, float)):
        actuals[:, 0] = actuals[:, 0] * v_sc
        actuals[:, 1:] = actuals[:, 1:] * r_sc
    elif isinstance(r_sc, TransformerMixin):
        predictions = r_sc.inverse_transform(predictions)
        actuals = r_sc.inverse_transform(actuals)
    else:
        predictions[:, 0] = predictions[:, 0] * v_sc[1] + v_sc[0]
        predictions[:, 1:] = predictions[:, 1:] * r_sc[1] + r_sc[0]

        actuals[:, 0] = actuals[:, 0] * v_sc[1] + v_sc[0]
        actuals[:, 1:] = actuals[:, 1:] * r_sc[1] + r_sc[0]

    predictions[predictions < 0] = 0

    # 创建一个新的DataFrame来存储实际值和预测值
    combined_df = pd.DataFrame(np.hstack((actuals, predictions)))

    # 定义列名
    column_names = ['Actual_' + str(i) for i in range(actuals.shape[1])] + ['Prediction_' + str(i) for i in
                                                                            range(predictions.shape[1])]
    combined_df.columns = column_names

    # 写入xlsx文件
    combined_df.to_excel('output.xlsx', index=False)

    # 计算指标
    mae = 0
    mae_mul = 0
    rmse = np.sqrt(mean_squared_error(actuals[:, 1:], predictions[:, 1:]))
    print(f"Root Mean Squared Error: {rmse}")
    mae = mean_absolute_error(actuals[:, 1:], predictions[:, 1:])
    mae_mul = mean_absolute_error(actuals[:, 1:], predictions[:, 1:],multioutput='raw_values')
    print(f"Mean Absolute Error (MAE): {mae}")
    r2 = r2_score(actuals[:, 0], predictions[:, 0])
    print(f"R^2 Score: {r2}")

    err_mean = np.mean(np.abs(predictions[:, 0] - actuals[:, 0]) / actuals[:, 0])
    print(f"mean error: {err_mean}")
    err_max = np.max(np.abs(predictions[:, 0] - actuals[:, 0]) / actuals[:, 0])
    print(f"max error: {err_max}")

    # 绘制散点图
    plt.figure(figsize=(10, 6))
    plt.scatter(actuals[:, 0], predictions[:, 0], alpha=1)
    # plt.plot([min(actuals[:,0]), max(actuals[:,0])], [min(actuals[:,0]), max(actuals[:,0])], 'r--')
    plt.plot([0, max(actuals[:, 0])], [0, max(actuals[:, 0])], 'r--')
    plt.xlabel('Actual Values')
    plt.ylabel('Predicted Values')
    plt.title('Predictions vs Actuals')
    plt.savefig('prediction_scatter.png')
    # plt.show(block=True) # 显示图像
    plt.close()

    return mae, r2, err_mean

if __name__ == '__main__':

    train_loader, test_loader, sc_x, sc_y = prepare_cav_build_data('dataset/cav_build_1.xlsx', batch_size=4260)

    device = torch.device("cuda:0" if torch.cuda.is_available() else "cpu")
    print(device)

    model = MyTransformer(num_layers=3, dim_feedforward=12, dropout=0.3) 
    #10.6：-relu，mean：0.60664，10.4:-relu，mean：0.5956，10.7：0.5929, 10.1：-relu，mean：0.5959,
    print(model)

    optimizer = torch.optim.Adam(model.parameters(), lr=0.001)
    # %%10.5：+relu，mean：0.597 10.6：-relu，mean：0.60664，10.4:-relu，mean：0.5956，10.7：0.5929
    model_path = 'models/model10.1.pt'
    if os.path.exists(model_path):
        dummy_input = torch.randn(1, 81) # 输入样例
        model.eval()

        traced_model = torch.jit.trace(model, dummy_input)
        traced_model.save("traced_model.pt")

        # 直接加载序列化的TorchScript模型
        model = torch.jit.load("traced_model.pt")
        print(model)

        key = Fernet.generate_key()
        cipher = Fernet(key)
        with open("traced_model.pt", "rb") as f:
            encrypted_data = cipher.encrypt(f.read())
        with open("model_encrypted.bin", "wb") as f:
            f.write(encrypted_data)

        # key = b"your_key_here"
        cipher = Fernet(key)

        # 读取并解密模型文件到内存
        with open("model_encrypted.bin", "rb") as f:
            encrypted_data = f.read()
        decrypted_data = cipher.decrypt(encrypted_data)

        # 直接加载 TorchScript 模型（无需定义 MyModel）
        buffer = BytesIO(decrypted_data)
        model = torch.jit.load(buffer)
        # model = torch.jit.load("traced_model1.pt")
        if torch.cuda.is_available():
            model.cuda()

        evaluate_model(model, test_loader, device, sc_x,sc_y)

    checkpoint_path = 'models/model10.1_last.pt'
    if os.path.exists(checkpoint_path):
        # model.load_state_dict(torch.load(checkpoint_path, map_location=device))
        checkpoint = torch.load(checkpoint_path)
        model.load_state_dict(checkpoint['model_state'])
        optimizer.load_state_dict(checkpoint['optimizer_state'])
        # start_epoch = checkpoint['epoch'] + 1

    if torch.cuda.is_available():
        model.cuda()

    model_save_dir = 'models/'
    model_save_path = os.path.join(model_save_dir, 'model10.1.pt')
    epochs = 1000000
    train_model(model, train_loader, test_loader, model_save_path, epochs, optimizer)
    writer.close()

    evaluate_model(model, test_loader, device, sc_x, sc_y)
