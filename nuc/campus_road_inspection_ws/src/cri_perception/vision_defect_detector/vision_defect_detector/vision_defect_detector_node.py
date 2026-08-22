#!/usr/bin/env python3
"""
视觉语义缺陷检测节点 - 基于YOLOv8+OpenVINO
订阅: /camera/image_raw (sensor_msgs/Image)
发布: /perception/vision_defects (cri_msgs/DefectArray)
发布: /perception/detection_image (sensor_msgs/Image) - 可视化
"""
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from cri_msgs.msg import DefectArray, DefectInfo
from geometry_msgs.msg import Point, Vector3
import numpy as np
import cv2
from cv_bridge import CvBridge
import json


class VisionDefectDetectorNode(Node):
    def __init__(self):
        super().__init__('vision_defect_detector_node')
        
        # 参数声明
        self.declare_parameter('model_path', '')
        self.declare_parameter('confidence_threshold', 0.5)
        self.declare_parameter('nms_threshold', 0.4)
        self.declare_parameter('input_size', 640)
        self.declare_parameter('device', 'CPU')
        self.declare_parameter('enable_visualization', True)
        self.declare_parameter('defect_id_prefix', 'vision')
        
        # 获取参数
        self.model_path = self.get_parameter('model_path').value
        self.conf_thresh = self.get_parameter('confidence_threshold').value
        self.nms_thresh = self.get_parameter('nms_threshold').value
        self.input_size = self.get_parameter('input_size').value
        self.device = self.get_parameter('device').value
        self.enable_viz = self.get_parameter('enable_visualization').value
        self.defect_id_prefix = self.get_parameter('defect_id_prefix').value
        
        # 类别名称（路面缺陷5类）
        self.class_names = ['crack', 'pothole', 'tile_uplift', 'depression', 'construction']
        
        # 初始化OpenVINO推理引擎
        self.model_loaded = False
        if self.model_path:
            self.init_openvino_model()
        else:
            self.get_logger().warn('未指定模型路径，将使用模拟检测')
        
        # CV Bridge
        self.bridge = CvBridge()
        
        # 订阅与发布
        self.image_sub = self.create_subscription(
            Image, '/camera/image_raw', self.image_callback, 10)
        self.defect_pub = self.create_publisher(DefectArray, '/perception/vision_defects', 10)
        
        if self.enable_viz:
            self.viz_pub = self.create_publisher(Image, '/perception/detection_image', 10)
        
        self.defect_counter = 0
        
        self.get_logger().info('视觉缺陷检测节点已启动')
        self.get_logger().info(f'置信度阈值: {self.conf_thresh}, 设备: {self.device}')
    
    def init_openvino_model(self):
        """初始化OpenVINO推理引擎"""
        try:
            from openvino.runtime import Core
            
            ie = Core()
            model = ie.read_model(model=self.model_path)
            self.compiled_model = ie.compile_model(model=model, device_name=self.device)
            
            self.input_layer = self.compiled_model.input(0)
            self.output_layer = self.compiled_model.output(0)
            
            self.model_loaded = True
            self.get_logger().info(f'OpenVINO模型加载成功: {self.model_path}')
            self.get_logger().info(f'输入形状: {self.input_layer.shape}')
            
        except ImportError:
            self.get_logger().error('OpenVINO未安装，请安装: pip install openvino')
        except Exception as e:
            self.get_logger().error(f'模型加载失败: {e}')
    
    def image_callback(self, msg: Image):
        """图像回调"""
        try:
            # 转换为OpenCV格式
            cv_image = self.bridge.imgmsg_to_cv2(msg, desired_encoding='bgr8')
        except Exception as e:
            self.get_logger().error(f'图像转换失败: {e}')
            return
        
        # 执行检测
        if self.model_loaded:
            detections = self.detect_with_openvino(cv_image)
        else:
            detections = self.detect_mock(cv_image)
        
        # 转换为缺陷消息
        defect_array = self.detections_to_defects(detections, msg.header, cv_image.shape)
        if len(defect_array.defects) > 0:
            self.defect_pub.publish(defect_array)
            self.get_logger().info(f'检测到 {len(defect_array.defects)} 个视觉缺陷')
        
        # 可视化
        if self.enable_viz:
            viz_image = self.visualize_detections(cv_image, detections)
            viz_msg = self.bridge.cv2_to_imgmsg(viz_image, encoding='bgr8')
            viz_msg.header = msg.header
            self.viz_pub.publish(viz_msg)
    
    def detect_with_openvino(self, image):
        """使用OpenVINO模型进行推理"""
        # 预处理
        input_tensor = self.preprocess_image(image)
        
        # 推理
        result = self.compiled_model([input_tensor])[self.output_layer]
        
        # 后处理
        detections = self.postprocess_yolov8(result, image.shape)
        
        return detections
    
    def preprocess_image(self, image):
        """YOLOv8预处理"""
        # Resize
        resized = cv2.resize(image, (self.input_size, self.input_size))
        
        # BGR to RGB
        rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
        
        # Normalize to [0, 1]
        normalized = rgb.astype(np.float32) / 255.0
        
        # HWC to CHW
        transposed = normalized.transpose(2, 0, 1)
        
        # Add batch dimension
        input_tensor = np.expand_dims(transposed, axis=0)
        
        return input_tensor
    
    def postprocess_yolov8(self, output, original_shape):
        """
        YOLOv8输出后处理
        output shape: [1, 84, 8400] for 80 classes COCO
        对于5类路面缺陷: [1, 9, 8400] (x, y, w, h, conf_class0, ..., conf_class4)
        """
        predictions = output[0].T  # [8400, 9]
        
        # 提取边界框和置信度
        boxes = predictions[:, :4]
        scores = predictions[:, 4:]
        
        # 获取每个检测的最高类别
        class_ids = np.argmax(scores, axis=1)
        confidences = np.max(scores, axis=1)
        
        # 过滤低置信度
        mask = confidences > self.conf_thresh
        boxes = boxes[mask]
        confidences = confidences[mask]
        class_ids = class_ids[mask]
        
        # 坐标缩放（从输入尺寸到原始图像尺寸）
        h, w = original_shape[:2]
        scale_x = w / self.input_size
        scale_y = h / self.input_size
        
        # 转换为 [x1, y1, x2, y2]
        scaled_boxes = []
        for box in boxes:
            cx, cy, bw, bh = box
            x1 = int((cx - bw / 2) * scale_x)
            y1 = int((cy - bh / 2) * scale_y)
            x2 = int((cx + bw / 2) * scale_x)
            y2 = int((cy + bh / 2) * scale_y)
            scaled_boxes.append([x1, y1, x2, y2])
        
        # NMS
        if len(scaled_boxes) > 0:
            keep_indices = self.nms(np.array(scaled_boxes), confidences)
            final_boxes = [scaled_boxes[i] for i in keep_indices]
            final_scores = confidences[keep_indices]
            final_classes = class_ids[keep_indices]
        else:
            final_boxes, final_scores, final_classes = [], [], []
        
        detections = []
        for box, score, cls in zip(final_boxes, final_scores, final_classes):
            detections.append({
                'bbox': box,
                'confidence': float(score),
                'class_id': int(cls),
                'class_name': self.class_names[int(cls)] if int(cls) < len(self.class_names) else 'unknown'
            })
        
        return detections
    
    def nms(self, boxes, scores):
        """非极大值抑制"""
        x1 = boxes[:, 0]
        y1 = boxes[:, 1]
        x2 = boxes[:, 2]
        y2 = boxes[:, 3]
        
        areas = (x2 - x1) * (y2 - y1)
        order = scores.argsort()[::-1]
        
        keep = []
        while order.size > 0:
            i = order[0]
            keep.append(i)
            
            xx1 = np.maximum(x1[i], x1[order[1:]])
            yy1 = np.maximum(y1[i], y1[order[1:]])
            xx2 = np.minimum(x2[i], x2[order[1:]])
            yy2 = np.minimum(y2[i], y2[order[1:]])
            
            w = np.maximum(0.0, xx2 - xx1)
            h = np.maximum(0.0, yy2 - yy1)
            inter = w * h
            
            iou = inter / (areas[i] + areas[order[1:]] - inter)
            
            inds = np.where(iou <= self.nms_thresh)[0]
            order = order[inds + 1]
        
        return keep
    
    def detect_mock(self, image):
        """模拟检测（用于测试）"""
        h, w = image.shape[:2]
        detections = [
            {
                'bbox': [int(w * 0.3), int(h * 0.3), int(w * 0.5), int(h * 0.5)],
                'confidence': 0.85,
                'class_id': 0,
                'class_name': 'crack'
            }
        ]
        return detections
    
    def detections_to_defects(self, detections, header, image_shape):
        """将检测结果转换为缺陷消息"""
        defect_array = DefectArray()
        defect_array.header = header
        
        for det in detections:
            self.defect_counter += 1
            defect = DefectInfo()
            defect.header = header
            defect.defect_id = f'{self.defect_id_prefix}_{self.defect_counter:06d}'
            defect.defect_type = det['class_name']
            defect.confidence = det['confidence']
            defect.detection_source = 'vision'
            
            # 边界框中心作为位置（像素坐标，需后续融合时转世界坐标）
            x1, y1, x2, y2 = det['bbox']
            cx = (x1 + x2) / 2.0
            cy = (y1 + y2) / 2.0
            defect.position = Point(x=float(cx), y=float(cy), z=0.0)
            
            # 尺寸（像素）
            width = float(x2 - x1)
            height = float(y2 - y1)
            defect.dimensions = Vector3(x=width, y=height, z=0.0)
            
            # 严重程度
            if det['confidence'] > 0.8:
                defect.severity_level = 'high'
            elif det['confidence'] > 0.6:
                defect.severity_level = 'medium'
            else:
                defect.severity_level = 'low'
            
            # 附加属性
            attributes = {
                'bbox_pixel': det['bbox'],
                'image_shape': list(image_shape[:2])
            }
            defect.attributes = json.dumps(attributes)
            
            defect_array.defects.append(defect)
        
        return defect_array
    
    def visualize_detections(self, image, detections):
        """可视化检测结果"""
        viz = image.copy()
        
        for det in detections:
            x1, y1, x2, y2 = det['bbox']
            conf = det['confidence']
            cls_name = det['class_name']
            
            # 绘制边界框
            color = (0, 255, 0)
            cv2.rectangle(viz, (x1, y1), (x2, y2), color, 2)
            
            # 绘制标签
            label = f'{cls_name}: {conf:.2f}'
            cv2.putText(viz, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 
                       0.5, color, 2)
        
        return viz


def main(args=None):
    rclpy.init(args=args)
    node = VisionDefectDetectorNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
