#!/usr/bin/env python
# coding: utf-8

# In[86]:


import numpy as np
import matplotlib.pyplot as plt
import os
import cv2

DATADIR = "/Users/sunnysahu/Desktop/ML_Project/Bitmap_Generator_Training_Set"
CATEGORIES = ["None", "Low", "Medium", "High"]

for category in CATEGORIES:
    path = os.path.join(DATADIR, category)
    for img in os.listdir(path):
        img_array = cv2.imread(os.path.join(path,img), cv2.IMREAD_GRAYSCALE)
        plt.imshow(img_array, cmap="gray")
        plt.show()
        break
    break
    


# In[87]:


print(img_array.shape)


# In[88]:


IMG_SIZE = 50

new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
plt.imshow(new_array, cmap="gray")
plt.show()


# In[89]:


training_data = []

def create_training_data():
    for category in CATEGORIES:
        path = os.path.join(DATADIR, category)
        class_num = CATEGORIES.index(category)
        for img in os.listdir(path):
            try:
                img_array = cv2.imread(os.path.join(path, img), cv2.IMREAD_GRAYSCALE)
                new_array = cv2.resize(img_array, (IMG_SIZE, IMG_SIZE))
                training_data.append([new_array, class_num])
            except Exception as e:
                pass

create_training_data()


# In[90]:


print(len(training_data))


# In[91]:


import random

random.shuffle(training_data)


# In[92]:


for sample in training_data:
    print(sample[1])


# In[93]:


X = []
y = []


# In[94]:


for features, label in training_data:
    X.append(features)
    y.append(label)

X = np.array(X).reshape(-1, IMG_SIZE, IMG_SIZE, 1)
y = np.array(y)


# In[95]:


import pickle

pickle_out = open("X.pickle", "wb")
pickle.dump(X, pickle_out)
pickle_out.close()

pickle_out = open("y.pickle", "wb")
pickle.dump(y, pickle_out)
pickle_out.close()


# In[ ]:




