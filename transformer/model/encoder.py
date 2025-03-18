# encoding = utf-8

from torch import nn
from model.embedding import Embedding
from model.multi_head_attention import MultiHeadAttention
from model.feed_forward import FeedForward
from model.layer_norm import LayerNorm

class EncoderLayer(nn.Module):
    '''
    Encoder layer: multi head attention and add norm.

    d_model: vector dimension after each token embedding
    ffn_hidden: feed Forward fidden Size, d_model * 4
    n_head: Number of Attention Heads
        d_model must be divided by n_head
    drop_prob: dropout probability
    '''
    def __init__(self, d_model, ffn_hidden, n_head, drop_prob):
        super(EncoderLayer, self).__init__()
        self.attention = MultiHeadAttention(d_model = d_model, n_head = n_head)
        self.norm1 = LayerNorm(d_model = d_model)
        self.dropout1 = nn.Dropout(p = drop_prob)

        self.ffn = FeedForward(d_model = d_model, hidden = ffn_hidden, drop_prob = drop_prob)
        self.norm2 = LayerNorm(d_model = d_model)
        self.dropout2 = nn.Dropout(p = drop_prob)

    def forward(self, x, src_mask):
        # 1 compute self attention
        _x = x
        x = self.attention(q = x, k = x, v = x, mask = src_mask)
        
        # 2 add and norm
        x = self.dropout1(x)
        x = self.norm1(x + _x)
        
        # 3 positionwise feed forward network
        _x = x
        x = self.ffn(x)
      
        # 4 add and norm
        x = self.dropout2(x)
        x = self.norm2(x + _x)
        return x

class Encoder(nn.Module):
    '''
    Encoder with embedding and encoder-layer.

    encoder_vocab_size: vocabulary size model can recognize
        BERT: 30000, GPT-3: 50000
    max_len: maximum sequence length on transformer can handle
        BERT: 512, GPT-2: 1024, GPT-3: 2048
    d_model: vector dimension after each token embedding
        Transformer-Base: 512, BERT-base: 768, BERT-large: 1024, GPT-3: 4096
    ffn_hidden: feed forward hidden size, usually d_model * 4
        Transformer-Base: 2048, BERT-base: 3072, BERT-large: 4096, GPT-3: 16384
    n_head: number of attention heads
        Transformer-Base: 8, BERT-base: 12, BERT-large: 16, GPT-3: 96
        d_model must be divided by n_head
    n_layers: number of encoder layers
        Small Transformer: 6, BERT-base: 12, BERT-large: 24, GPT-3: 96
    drop_prob: dropout probability
    device: torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    '''
    def __init__(self, max_len, d_model, ffn_hidden, encoder_vocab_size, n_head, n_layer, drop_prob, device):
        super().__init__()
        self.embedding = Embedding(d_model = d_model,
                                max_len = max_len,
                                vocab_size = encoder_vocab_size,
                                drop_prob = drop_prob,
                                device = device)

        self.layers = nn.ModuleList([EncoderLayer(d_model = d_model,
                                            ffn_hidden = ffn_hidden,
                                            n_head = n_head,
                                            drop_prob = drop_prob)
                                     for _ in range(n_layer)])

    def forward(self, x, src_mask):
        x = self.embedding(x)

        for layer in self.layers:
            x = layer(x, src_mask)

        return x
