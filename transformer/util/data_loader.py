# encoding = utf-8
# load eng-deu test dataset

import os
import torch
from torch.utils.data import Dataset
from torch.utils.data import DataLoader as TorchDataLoader
from collections import Counter
import spacy
from urllib.request import urlretrieve
import zipfile

class DataLoader:
    source = None
    target = None

    def __init__(self, ext, tokenize_en, tokenize_de, init_token, eos_token):
        self.ext = ext
        self.tokenize_en = tokenize_en
        self.tokenize_de = tokenize_de
        self.init_token = init_token
        self.eos_token = eos_token
        print('dataset initializing start')

    def make_dataset(self):
        """下载数据集并进行分割"""
        train_data, valid_data, test_data = self._download_and_split_data()
        return train_data, valid_data, test_data

    def build_vocab(self, train_data, min_freq):
        """构建 source 和 target 词汇表"""
        self.source = self._build_vocab([src for src, _ in train_data], min_freq)
        self.target = self._build_vocab([tgt for _, tgt in train_data], min_freq)

    def make_iter(self, train, validate, test, batch_size, device):
        """创建数据迭代器"""
        train_loader = self._create_dataloader(train, batch_size, device)
        valid_loader = self._create_dataloader(validate, batch_size, device)
        test_loader = self._create_dataloader(test, batch_size, device)
        print('dataset initializing done')
        return train_loader, valid_loader, test_loader

    def _download_and_split_data(self):
        """下载并拆分数据集"""
        data_dir = "./data"
        os.makedirs(data_dir, exist_ok=True)
        data_path = os.path.join(data_dir, "deu.txt")

        # 下载数据集
        if not os.path.exists(data_path):
            zip_path = os.path.join(data_dir, "deu-eng.zip")
            urlretrieve("https://www.manythings.org/anki/deu-eng.zip", zip_path)
            with zipfile.ZipFile(zip_path, "r") as zip_ref:
                zip_ref.extractall(data_dir)
        
        with open(data_path, "r", encoding="utf-8") as f:
            lines = f.readlines()

        # 数据切分
        split_ratio = [0.8, 0.1, 0.1]
        train_size = int(len(lines) * split_ratio[0])
        valid_size = int(len(lines) * split_ratio[1])

        train_lines = lines[:train_size]
        valid_lines = lines[train_size:train_size + valid_size]
        test_lines = lines[train_size + valid_size:]

        train_data = [(self.tokenize_de(src), self.tokenize_en(tgt)) for src, tgt, _ in (l.strip().split("\t") for l in train_lines)]
        valid_data = [(self.tokenize_de(src), self.tokenize_en(tgt)) for src, tgt, _ in (l.strip().split("\t") for l in valid_lines)]
        test_data = [(self.tokenize_de(src), self.tokenize_en(tgt)) for src, tgt, _ in (l.strip().split("\t") for l in test_lines)]

        print("Data successfully split into train/valid/test.")
        return train_data, valid_data, test_data

    def _build_vocab(self, data, min_freq):
        """构建词汇表"""
        counter = Counter()
        for sentence in data:
            counter.update(sentence)

        stoi = {"<pad>": 0, "<sos>": 1, "<eos>": 2, "<unk>": 3}
        itos = ["<pad>", "<sos>", "<eos>", "<unk>"]
        
        for word, freq in counter.items():
            if freq >= min_freq:
                stoi[word] = len(itos)
                itos.append(word)

        print(f"Vocab built with {len(itos)} words.")
        return {"stoi": stoi, "itos": itos}

    def _collate_fn(self, batch):
        """批处理函数，进行动态填充"""
        src_batch, tgt_batch = zip(*batch)

        src_batch = [[self.source["stoi"]["<sos>"]] + [self.source["stoi"].get(word, self.source["stoi"]["<unk>"]) for word in sent] + [self.source["stoi"]["<eos>"]] for sent in src_batch]
        tgt_batch = [[self.target["stoi"]["<sos>"]] + [self.target["stoi"].get(word, self.target["stoi"]["<unk>"]) for word in sent] + [self.target["stoi"]["<eos>"]] for sent in tgt_batch]

        max_src_len = max(len(sent) for sent in src_batch)
        max_tgt_len = max(len(sent) for sent in tgt_batch)

        src_batch_padded = [sent + [self.source["stoi"]["<pad>"]] * (max_src_len - len(sent)) for sent in src_batch]
        tgt_batch_padded = [sent + [self.target["stoi"]["<pad>"]] * (max_tgt_len - len(sent)) for sent in tgt_batch]

        return torch.tensor(src_batch_padded), torch.tensor(tgt_batch_padded)

    def _create_dataloader(self, data, batch_size, device):
        """创建 DataLoader"""
        dataset = CustomDataset(data)
        return TorchDataLoader(dataset, batch_size = batch_size, 
                            shuffle = True, collate_fn = self._collate_fn)
    
class CustomDataset(Dataset):
    """自定义数据集"""
    def __init__(self, data):
        self.data = data

    def __len__(self):
        return len(self.data)

    def __getitem__(self, idx):
        return self.data[idx]

