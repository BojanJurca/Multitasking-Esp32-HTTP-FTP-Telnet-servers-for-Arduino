/*
 * 
 * Measurements.hpp 
 * 
 *  This file is part of Esp32_web_ftp_telnet_server_template project: https://github.com/BojanJurca/Esp32_web_ftp_telnet_server_template
 * 
 *  Measurements.hpp include circular queue for storing measurements data set.
 * 
 * History:
 *          - first release, 
 *            October 31, 2018, Bojan Jurca
 *  
 */


typedef struct measurementType {
   unsigned char scale;    
   int           value;
};

portMUX_TYPE csMeasurementsInternalStructure = portMUX_INITIALIZER_UNLOCKED;


class measurements {                                             

  public:

    measurements (int noOfSamples)            {                  // constructor
                                                if (this->__measurements__ = (measurementType *) malloc ((noOfSamples + 1) * sizeof (measurementType))) { // one space more then the number of measurements in the queue
                                                  this->__noOfSamples__ = noOfSamples;
                                                  this->__beginning__ = this->__end__ = 0;
                                                }
                                              }

    ~measurements ()                          {                 // destructor
                                                if (this->__measurements__) free (this->__measurements__);
                                              } 

    void addMeasurement (unsigned char scale, int value)      // add (measurement, scale) into circular queue
                                              { 
                                                if (!this->__measurements__) return;
                                                portENTER_CRITICAL (&csMeasurementsInternalStructure);
                                                  *(this->__measurements__ + this->__end__) = {scale, value};
                                                  this->__end__ = (this->__end__ + 1) % (this->__noOfSamples__ + 1); 
                                                  if (this->__end__ == this->__beginning__) this->__beginning__ = (this->__beginning__ + 1) % (this->__noOfSamples__ + 1); 
                                                portEXIT_CRITICAL (&csMeasurementsInternalStructure);
                                              }  

    void increaseCounter ()                   {               // increase internal counter (that is not added to measurements yet))
                                                if (!this->__measurements__) return;
                                                portENTER_CRITICAL (&csMeasurementsInternalStructure);
                                                  this->__counter__++;
                                                portEXIT_CRITICAL (&csMeasurementsInternalStructure);
                                              }                                               

    void addCounterToMeasurements (unsigned char scale)         // add (counter, scale) into circular queue and resets counter
                                              { 
                                                if (!this->__measurements__) return;
                                                portENTER_CRITICAL (&csMeasurementsInternalStructure);
                                                  *(this->__measurements__ + this->__end__) = {scale, this->__counter__};
                                                  this->__counter__ = 0;
                                                  this->__end__ = (this->__end__ + 1) % (this->__noOfSamples__ + 1); 
                                                  if (this->__end__ == this->__beginning__) this->__beginning__ = (this->__beginning__ + 1) % (this->__noOfSamples__ + 1); 
                                                portEXIT_CRITICAL (&csMeasurementsInternalStructure);
                                              }  
                                              
    String measurements2json (int scaleModule){                 // returns json structure of measurements
                                                struct measurementType tmp;
                                                String s = "{\"scale\":[";
                                                portENTER_CRITICAL (&csMeasurementsInternalStructure);
                                                  int i = this->__beginning__;
                                                  while (i != this->__end__) {
                                                    if (i != this->__beginning__) s += ",";
                                                    tmp = *(this->__measurements__ + i);
                                                    if (tmp.scale == 255 || scaleModule && (tmp.scale % scaleModule != 0)) {s += "\"\"";} // skip all 255 scales and if module is specified skip all scales where module is not 0
                                                    else                                                                   {s += "\"" + String (tmp.scale) + "\"";}
                                                    i = (i + 1) % (this->__noOfSamples__ + 1); 
                                                  }
                                                  s += "],\"value\":[";
                                                  i = this->__beginning__;
                                                  while (i != this->__end__) {
                                                    if (i != this->__beginning__) s += ",";
                                                    tmp = *(this->__measurements__ + i);
                                                    s += "\"" + String (tmp.value) + "\"";
                                                    i = (i + 1) % (this->__noOfSamples__ + 1); 
                                                  }
                                                portEXIT_CRITICAL (&csMeasurementsInternalStructure);
                                                s += "]}\r\n";
                                                return s;
                                              }
 
  private:

    measurementType *__measurements__;                            // pointer to measuremnet queue 
    int __noOfSamples__;                                          // max number of measurements in the queue
    int __beginning__;                                            // last occupied location
    int __end__;                                                  // first free location (if it is the same as __beginning__ then the queue is empty)
    int __counter__ = 0;
                                            
};
