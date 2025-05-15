import pandas as pd
from sklearn.preprocessing import StandardScaler
from sklearn.cluster import DBSCAN
from sklearn.metrics import classification_report, confusion_matrix
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

'''
hints:
    如果检测到的异常点太少(或没有)
        尝试减小 eps
        尝试减小 min_samples

    如果检测到的异常点太多(很多正常点被误报)
        尝试增大 eps
        尝试增大 min_samples
'''

def load_data(file_path):
    '''
    parameter:
        file_path (str): 数据集文件的路径.
    return:
        pandas.DataFrame: 加载的数据集.
    '''
    try:
        data = pd.read_csv(file_path)
        print(f'Load data succeed, shape: {data.shape}')
        data.info()
        print('Data distribution:')
        print(data['Class'].value_counts())
        return data
    except FileNotFoundError:
        print(f'Error: "{file_path}" not found')
        return None

def preprocess_data(df):
    '''
    process data, normalize 'Amount' and 'Time'.
    parameter:
        df (pandas.DataFrame): input data frame.
    return:
        pandas.DataFrame: 经过预处理的数据框副本（仅包含特征）.
        pandas.Series: 真实的标签.
    '''
    if df is None:
        return None, None

    processed_df = df.copy()

    # remove 'Time'
    if 'Time' in processed_df.columns:
        processed_df = processed_df.drop('Time', axis=1)

    # normalize 'Amount'
    if 'Amount' in processed_df.columns:
        scaler = StandardScaler()
        processed_df['Amount'] = scaler.fit_transform(processed_df['Amount'].values.reshape(-1, 1))
        print('"Amount" normalized.')

    # separate label and feature
    X = processed_df.drop('Class', axis=1)
    y_true = processed_df['Class']

    return X, y_true

def run_dbscan_anomaly_detection(X, eps_param, min_samples_param):
    '''
    parameter:
        X (pandas.DataFrame): 预处理后的特征数据.
        eps_param (float): DBSCAN 的 eps 参数 (邻域半径).
        min_samples_param (int): DBSCAN 的 min_samples 参数 (形成核心对象的最小样本数).
    return:
        numpy.ndarray: DBSCAN 预测的标签 (-1 表示异常点, >=0 表示正常簇).
    '''
    if X is None:
        return None

    print(f'DBSCAN parameter: eps={eps_param}, min_samples={min_samples_param}')

    dbscan = DBSCAN(eps=eps_param, min_samples=min_samples_param, n_jobs=-1)
    dbscan_labels = dbscan.fit_predict(X)

    # DBSCAN noise point(abnormal point) is -1
    num_outliers = (dbscan_labels == -1).sum()
    num_clusters = len(set(dbscan_labels)) - (1 if -1 in dbscan_labels else 0)

    print(f'num_clusters: {num_clusters}')
    print(f'num_outliers: {num_outliers}')

    return dbscan_labels

def evaluate_and_visualize_results(y_true, dbscan_labels, X_eval=None, plot=False):
    '''
    parameter:
        y_true (pandas.Series): 真实的标签.
        dbscan_labels (numpy.ndarray): DBSCAN 预测的标签.
        X_eval (pandas.DataFrame, 可选): 用于可视化的特征数据.
    '''
    if y_true is None or dbscan_labels is None:
        print('无法评估, 输入数据不完整.')
        return

    # convert DBSCAN labels, 0: normal, 1: abnoraml
    # -1 abnormal, other normal
    y_pred_dbscan = np.zeros_like(dbscan_labels)
    y_pred_dbscan[dbscan_labels == -1] = 1 # change abnormals from -1 to 1

    # calculate confusion matrix
    cm = confusion_matrix(y_true, y_pred_dbscan)
    print('confusion matrix:')
    print(cm)

    try:
        report = classification_report(y_true, y_pred_dbscan, target_names=['正常 (0)', '欺诈 (1)'], output_dict=False, zero_division=0)
        print(report)
    except Exception as e:
        print(f'Error print report: {e}')

    if plot:  
        if X_eval is not None and X_eval.shape[1] >= 2:
            plt.figure(figsize=(12, 8))
            feature1 = X_eval.columns[0] if X_eval.columns[0] else 'V1'
            feature2 = X_eval.columns[1] if X_eval.columns[1] else 'V2'

            # normal points
            normal_points = X_eval[dbscan_labels != -1]
            # abnormal points(noise points)
            outlier_points = X_eval[dbscan_labels == -1]

            plt.scatter(normal_points[feature1], normal_points[feature2],
                    c=dbscan_labels[dbscan_labels != -1], cmap='viridis', s=20, label='Normal points in cluster')
            plt.scatter(outlier_points[feature1], outlier_points[feature2],
                    c='red', marker='x', s=50, label='DBSCAN abnormal points (-1)')

            # label real abnoraml points
            true_frauds = X_eval[y_true == 1]
            plt.scatter(true_frauds[feature1], true_frauds[feature2],
                    facecolors='none', edgecolors='black', marker='o', s=100, label='True fraud points')

            plt.title(f'DBSCAN clustering result (Feature: {feature1} vs {feature2})\nRed "x" is abnormal points detected by DBSCAN')
            plt.xlabel(feature1)
            plt.ylabel(feature2)
            plt.legend()
            plt.grid(True)
            plt.savefig('res.png')
        else:
            print('Cannot visualize, X_eval not exist or dimension is not enough.')

if __name__ == '__main__':

    DATA_FILE_PATH = 'dataset/creditcard.csv'

    SAMPLE_FRACTION = 0.8 # data used portion
    USE_SAMPLE = True
    # DBSCAN_EPS = 1.5
    # DBSCAN_MIN_SAMPLES = 50

    current_eps = 0.75
    current_min_samples = 16

    # 1 load data
    credit_card_data = load_data(DATA_FILE_PATH)

    # 2 pre process data
    X_features, y_labels = preprocess_data(credit_card_data)

    if USE_SAMPLE and SAMPLE_FRACTION < 1.0:
        # print(f'\n由于数据集较大, 将使用 {SAMPLE_FRACTION * 100:.1f}% 的随机子样本进行DBSCAN.')
        sample_indices = np.random.choice(X_features.index,
                                            size=int(len(X_features) * SAMPLE_FRACTION),
                                            replace=False)
        X_sample = X_features.loc[sample_indices]
        y_sample_true = y_labels.loc[sample_indices]
    else:
        X_sample = X_features
        y_sample_true = y_labels

    print(f'training data shape: {X_sample.shape}')

    # 3 run DBSCAN
    print('Start running DBSCAN...')
    predicted_labels = run_dbscan_anomaly_detection(X_sample,
                                                eps_param=current_eps,
                                                min_samples_param=current_min_samples)

    # 4 assess and optimization
    evaluate_and_visualize_results(y_sample_true, predicted_labels, X_sample)

    # further analysis
    if (predicted_labels == -1).sum() > 0:
        true_positives_dbscan = y_sample_true[predicted_labels == -1].sum()
        detected_outliers_count = (predicted_labels == -1).sum()
        print(f'Total {detected_outliers_count} abnormal points deteced, {true_positives_dbscan} are real.')
        if detected_outliers_count > 0 :
            precision_at_outliers = true_positives_dbscan / detected_outliers_count
            print(f'Precision for outliers: {precision_at_outliers:.4f}')

        total_true_frauds_in_sample = y_sample_true.sum()
        if total_true_frauds_in_sample > 0:
            recall_for_frauds = true_positives_dbscan / total_true_frauds_in_sample
            print(f'Recall for frauds based on outliers: {recall_for_frauds:.4f}')
    else:
        print('\nNo outliers detected.')
