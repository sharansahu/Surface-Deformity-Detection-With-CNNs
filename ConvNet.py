#!/usr/bin/env python
# coding: utf-8

# In[7]:


import tensorflow as tf
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import Dense, Dropout, Activation, Flatten, Conv2D, MaxPooling2D
import pickle
from keras.utils import to_categorical
import matplotlib.pyplot as plt
import numpy as np

X = pickle.load(open("X.pickle", "rb"))
y = pickle.load(open("y.pickle", "rb"))

y = to_categorical(y)

X = X/255.0

model = Sequential()
model.add(Conv2D(64, (3,3), activation='relu', input_shape=X.shape[1:]))
model.add(MaxPooling2D(pool_size=(2,2)))

model.add(Conv2D(64, (3,3), activation='relu'))
model.add(MaxPooling2D(pool_size=(2,2)))

model.add(Flatten())
model.add(Dense(64, activation='relu'))
model.add(Dense(4, activation='softmax'))

model.compile(loss='categorical_crossentropy', optimizer='adam', metrics=['accuracy'])

hist = model.fit(X, y, batch_size = 32, epochs = 100, validation_split=0.1)


# In[12]:


model.evaluate(X, y)


# In[13]:


plt.plot(hist.history['accuracy'])
plt.plot(hist.history['val_accuracy'])
plt.title("Model Accuracy")
plt.ylabel("Accuracy")
plt.xlabel("Epoch")
plt.legend(["Train", "Val"], loc='upper left')
plt.show()

plt.plot(hist.history['loss'])
plt.plot(hist.history['val_loss'])
plt.title("Model Loss")
plt.ylabel("Loss")
plt.xlabel("Epoch")
plt.legend(["Train", "Val"], loc='upper left')
plt.show()


# In[14]:


predictions = model.predict(X[:15])
predictions


# In[15]:


predictions_array = np.argmax(predictions, axis = 1)
print("Predictions:")
print(predictions_array)
for i in range(0,15):
    image = X[i]
    image = np.array(image, dtype = 'float')
    pixels = image.reshape((50,50))
    plt.imshow(pixels, cmap='gray')
    plt.show()
    print(y[i])
    
    if (predictions_array[i] == 3):
        print("This deformity will have a high impact on fuel economy")
        
    if (predictions_array[i] == 2):
        print("This deformity will have a medium impact on fuel economy")
        
    if (predictions_array[i] == 1):
        print("This deformity will have a low impact on fuel economy")
        
    if (predictions_array[i] == 0):
        print("This deformity will have no impact on fuel economy")
    


# In[ ]:





# In[ ]:




