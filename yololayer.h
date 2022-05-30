#ifndef _YOLO_LAYER_H
#define _YOLO_LAYER_H

#include <vector>
#include <string>
#include <NvInfer.h>
#include "macros.h"

namespace Yolo
{
    static constexpr int CHECK_COUNT = 3;
    static constexpr float IGNORE_THRESH = 0.1f;
    struct YoloKernel
    {
        int width;
        int height;
        float anchors[CHECK_COUNT * 2];
    };
    static constexpr int MAX_OUTPUT_BBOX_COUNT = 1000;
    static constexpr int CLASS_NUM = 80;
    static constexpr int INPUT_H = 640;  // yolov5's input height and width must be divisible by 32.
    static constexpr int INPUT_W = 640;

    static constexpr int LOCATIONS = 4; 
	//alignas���������ڴ��еĶ��뷽ʽ����С��8�ֽڶ���(Ĭ��)��������16��32��64��128��
    struct alignas(float) Detection {
        //center_x center_y w h
        float bbox[LOCATIONS];
        float conf;  // bbox_conf * cls_conf
        float class_id;
    };
}

namespace nvinfer1
{
    class API YoloLayerPlugin : public IPluginV2IOExt
    {
    public:
        YoloLayerPlugin(int classCount, int netWidth, int netHeight, int maxOut, const std::vector<Yolo::YoloKernel>& vYoloKernel);
        YoloLayerPlugin(const void* data, size_t length);
        ~YoloLayerPlugin();

		//getNbOutputs��ȡlayer���������
        int getNbOutputs() const TRT_NOEXCEPT override
        {
            return 1;
        }
		//getOutputDimensions��ȡlayer�����ά��
        Dims getOutputDimensions(int index, const Dims* inputs, int nbInputDims) TRT_NOEXCEPT override;
		//initialize()��ִ��ʱ��ʼ��layer����engine����ʱ����
        int initialize() TRT_NOEXCEPT override;
		//terminate()�ͷų�ʼ��layerʱ��ϵͳ��Դ����engine����ʱ����
        virtual void terminate() TRT_NOEXCEPT override {};
		//getWorkspaceSize��ȡlayer����ռ��С�����������getSerializationSize��ͬ������ָ�����ݺͲ���������layer�Ķ���ռ䡣
        virtual size_t getWorkspaceSize(int maxBatchSize) const TRT_NOEXCEPT override { return 0; }
		//$$$����Ҫ������enqueueִ��layer��һϵ�в���������ֵ0��1����ִ���Ƿ�ɹ�����layer���ݲ�������������������ڣ�
        virtual int enqueue(int batchSize, const void* const* inputs, void*TRT_CONST_ENQUEUE* outputs, void* workspace, cudaStream_t stream) TRT_NOEXCEPT override;
		//size_t getSerializationSize()�����model�Ŀռ��С
        virtual size_t getSerializationSize() const TRT_NOEXCEPT override;
		//serialize���л�һ��layer
        virtual void serialize(void* buffer) const TRT_NOEXCEPT override;
		//supportsFormatCombination�ж�plugin�Ƿ�֧��������������͡�
        bool supportsFormatCombination(int pos, const PluginTensorDesc* inOut, int nbInputs, int nbOutputs) const TRT_NOEXCEPT override {
            return inOut[pos].format == TensorFormat::kLINEAR && inOut[pos].type == DataType::kFLOAT;
        }

        const char* getPluginType() const TRT_NOEXCEPT override;

        const char* getPluginVersion() const TRT_NOEXCEPT override;

        void destroy() TRT_NOEXCEPT override;

        IPluginV2IOExt* clone() const TRT_NOEXCEPT override;
		//����plugin�������ռ�
        void setPluginNamespace(const char* pluginNamespace) TRT_NOEXCEPT override;
		//��ȡplugin�������ռ�
        const char* getPluginNamespace() const TRT_NOEXCEPT override;
		//��������������
        DataType getOutputDataType(int index, const nvinfer1::DataType* inputTypes, int nbInputs) const TRT_NOEXCEPT override;
		//�����������������й㲥���򷵻� true��
        bool isOutputBroadcastAcrossBatch(int outputIndex, const bool* inputIsBroadcasted, int nbInputs) const TRT_NOEXCEPT override;
		//���plugin����ʹ�ÿ����ι㲥�����踴�Ƶ����룬�򷵻� true��
        bool canBroadcastInputAcrossBatch(int inputIndex) const TRT_NOEXCEPT override;
		//��plugin���ӵ�ִ�������Ĳ�����plugin��ĳЩ��������Դ�ķ���Ȩ��
        void attachToContext(
            cudnnContext* cudnnContext, cublasContext* cublasContext, IGpuAllocator* gpuAllocator) TRT_NOEXCEPT override;
		/*
		����layer������������������������������������ά�Ⱥ��������͡��������������Ĺ㲥��Ϣ��
		ѡ��Ĳ����ʽ�����������С����ʱ������������ڲ�״̬��Ϊ��������ѡ������ʵ��㷨�����ݽṹ��
		*/
        void configurePlugin(const PluginTensorDesc* in, int nbInput, const PluginTensorDesc* out, int nbOutput) TRT_NOEXCEPT override;
		//������������ִ���������з������
        void detachFromContext() TRT_NOEXCEPT override;

    private:
        void forwardGpu(const float* const* inputs, float *output, cudaStream_t stream, int batchSize = 1);
        int mThreadCount = 256;
        const char* mPluginNamespace;
        int mKernelCount;
        int mClassCount;
        int mYoloV5NetWidth;
        int mYoloV5NetHeight;
        int mMaxOutObject;
        std::vector<Yolo::YoloKernel> mYoloKernel;
        void** mAnchor;
    };

    class API YoloPluginCreator : public IPluginCreator
    {
    public:
        YoloPluginCreator();

        ~YoloPluginCreator() override = default;

        const char* getPluginName() const TRT_NOEXCEPT override;

        const char* getPluginVersion() const TRT_NOEXCEPT override;

        const PluginFieldCollection* getFieldNames() TRT_NOEXCEPT override;

        IPluginV2IOExt* createPlugin(const char* name, const PluginFieldCollection* fc) TRT_NOEXCEPT override;

        IPluginV2IOExt* deserializePlugin(const char* name, const void* serialData, size_t serialLength) TRT_NOEXCEPT override;

        void setPluginNamespace(const char* libNamespace) TRT_NOEXCEPT override
        {
            mNamespace = libNamespace;
        }

        const char* getPluginNamespace() const TRT_NOEXCEPT override
        {
            return mNamespace.c_str();
        }

    private:
        std::string mNamespace;
        static PluginFieldCollection mFC;
        static std::vector<PluginField> mPluginAttributes;
    };
    REGISTER_TENSORRT_PLUGIN(YoloPluginCreator);
};

#endif  // _YOLO_LAYER_H
