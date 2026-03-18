"""스케치 이미지 전용 데이터 증강 파이프라인.

스케치(아동화) 특성에 맞는 증강만 적용:
- RandomAffine: 스캔 각도/위치 차이 보정
- RandomHorizontalFlip: 왼손잡이 아동의 거울상
- RandomPerspective: 종이 평면 아닌 경우
- fill=255: 스케치 배경은 흰색

사용하지 않는 증강:
- VerticalFlip: 뒤집힌 사람은 다른 그림
- ColorJitter: 스케치는 흑백, 색 변환 무의미
"""

from torchvision import transforms


def get_train_transform(input_size: int = 260) -> transforms.Compose:
    """학습용 증강 파이프라인 반환.

    Args:
        input_size: 모델 입력 크기 (기본 260 = EfficientNet-B2)

    Returns:
        torchvision transforms.Compose 객체
    """
    return transforms.Compose([
        transforms.Resize((input_size, input_size)),
        transforms.RandomHorizontalFlip(p=0.5),
        transforms.RandomAffine(
            degrees=15,
            translate=(0.10, 0.10),
            scale=(0.85, 1.15),
            fill=255,  # 흰색 배경
        ),
        transforms.RandomPerspective(
            distortion_scale=0.10,
            p=0.3,
            fill=255,
        ),
        transforms.ToTensor(),
        transforms.Normalize(
            mean=(0.972, 0.031, 0.012),
            std=(0.156, 0.174, 0.074),
        ),
    ])


def get_val_transform(input_size: int = 260) -> transforms.Compose:
    """검증/추론용 변환 파이프라인 반환 (증강 없음).

    Args:
        input_size: 모델 입력 크기

    Returns:
        torchvision transforms.Compose 객체
    """
    return transforms.Compose([
        transforms.Resize((input_size, input_size)),
        transforms.ToTensor(),
        transforms.Normalize(
            mean=(0.972, 0.031, 0.012),
            std=(0.156, 0.174, 0.074),
        ),
    ])
