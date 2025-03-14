# encoding = utf-8

from torch import nn

from model.layer_norm import LayerNorm
from model.multi_head_attention import MultiHeadAttention
from model.feed_forward import FeedForward
from model.embedding import Embedding

class DecoderLayer(nn.Module):

    def __init__(self, d_model, ffn_hidden, n_head, drop_prob):
        super(DecoderLayer, self).__init__()
        self.self_attention = MultiHeadAttention(d_model = d_model, n_head = n_head)
        self.norm1 = LayerNorm(d_model = d_model)
        self.dropout1 = nn.Dropout(p = drop_prob)

        self.encoder_decoder_attention = MultiHeadAttention(d_model = d_model, n_head = n_head)
        self.norm2 = LayerNorm(d_model = d_model)
        self.dropout2 = nn.Dropout(p = drop_prob)

        self.ffn = FeedForward(d_model=d_model, hidden=ffn_hidden, drop_prob=drop_prob)
        self.norm3 = LayerNorm(d_model=d_model)
        self.dropout3 = nn.Dropout(p=drop_prob)

    def forward(self, dec, enc, trg_mask, src_mask):
        # 1. compute self attention
        _x = dec
        x = self.self_attention(q=dec, k=dec, v=dec, mask=trg_mask)
        
        # 2. add and norm
        x = self.dropout1(x)
        x = self.norm1(x + _x)

        if enc is not None:
            # 3. compute encoder - decoder attention
            _x = x
            x = self.encoder_decoder_attention(q=x, k=enc, v=enc, mask=src_mask)
            
            # 4. add and norm
            x = self.dropout2(x)
            x = self.norm2(x + _x)

        # 5. positionwise feed forward network
        _x = x
        x = self.ffn(x)
        
        # 6. add and norm
        x = self.dropout3(x)
        x = self.norm3(x + _x)
        return x

class Decoder(nn.Module):
    def __init__(self, dec_voc_size, max_len, d_model, ffn_hidden, n_head, n_layers, drop_prob, device):
        super().__init__()
        self.embedding = Embedding(d_model = d_model,
                                drop_prob = drop_prob,
                                max_len = max_len,
                                vocab_size = dec_voc_size,
                                device = device)

        self.layers = nn.ModuleList([DecoderLayer(d_model = d_model,
                                                ffn_hidden = ffn_hidden,
                                                n_head = n_head,
                                                drop_prob = drop_prob)
                                     for _ in range(n_layers)])

        self.linear = nn.Linear(d_model, dec_voc_size)

    def forward(self, trg, enc_src, trg_mask, src_mask):
        trg = self.embedding(trg)

        for layer in self.layers:
            trg = layer(trg, enc_src, trg_mask, src_mask)

        # pass to LM head
        output = self.linear(trg)
        return output

