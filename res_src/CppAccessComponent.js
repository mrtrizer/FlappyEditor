class CppAccessComponent extends Component {
    setAngleToTransform(angle) {
        this.entity().component('TransformComponent').setAngle2DRad(angle);
    }
    getAngleFromTransform() {
        return this.entity().component('TransformComponent').angle2DRad();
    }
    setPosToTransform(pos) {
        this.entity().component('TransformComponent').setPos(pos);
    }
    getPosFromTransform() {
        return this.entity().component('TransformComponent').pos();
    }
    createInternal() {
        this.entity().addComponent(new InternalComponent());
    }
    setInternalValue(value) {
        this.entity().component('InternalComponent').setValue(value);
    }
    getInternalValue() {
        return this.entity().component('InternalComponent').getValue();
    }
}
