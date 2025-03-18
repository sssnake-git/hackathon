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
    '''
    Decoder with embedding and decoder-layer.

    vocab_size: vocabulary size model can recognize
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
    def __init__(self, decoder_vocab_size, max_len, d_model, ffn_hidden, n_head, n_layer, drop_prob, device):
        super().__init__()
        self.embedding = Embedding(d_model = d_model,
                                drop_prob = drop_prob,
                                max_len = max_len,
                                vocab_size = decoder_vocab_size,
                                device = device)

        self.layers = nn.ModuleList([DecoderLayer(d_model = d_model,
                                                ffn_hidden = ffn_hidden,
                                                n_head = n_head,
                                                drop_prob = drop_prob)
                                     for _ in range(n_layer)])

        self.linear = nn.Linear(d_model, decoder_vocab_size)

    def forward(self, target, enc_src, target_mask, src_mask):
        '''
        target: target sequence input, tokenized sequence of target language
            shape: (batch_size, target_seq_len)
        encoder_src: encoder hidden states
            shape: (batch_size, src_seq_len, d_model)
        target_mask: target sequence mask, prevent decoder foresees future info during training
            shape: (batch_size, target_seq_len)
        src_mask: source sequence mask, ignore `<PAD>` tokens from input, prevent info leak
        '''
        target = self.embedding(target)

        for layer in self.layers:
            target = layer(target, enc_src, target_mask, src_mask)

        # pass to LM head
        output = self.linear(target)
        return output

